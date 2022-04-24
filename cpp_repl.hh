
#ifndef CPP_REPL
#define CPP_REPL

#include <string>
#include <map>
#include <vector>
#include <stdexcept>

namespace repl
{
    class exit : public std::exception
    {};

    // Thrown when an error occurs while calling before
    // or after handlers
    class handler_error : public std::runtime_error
    {
        std::string msg;
        std::exception cause_e;
    public:
        handler_error(const std::string& msg, const std::exception& cause)
        : std::runtime_error{msg.c_str()}, msg{msg + " => " + cause.what()}, cause_e{cause}
        {}

        virtual const char* what() const noexcept override
        {
            return msg.c_str();
        }
    };

    template <typename T>
    class command
    {
    public:
        using cmd_ptr = void (T::*)(const std::string&);
    private:
        std::string nm;
        std::string hlp;
        cmd_ptr cmd;

    public:
        command() = default;

        command(const command&) = default;
        command(command&&) = default;

        command& operator=(const command&) = default;
        command& operator=(command&&) = default;

        command(const std::string& name, cmd_ptr command, const std::string& help)
        : nm{name}, hlp{help},  cmd{command}
        {}

        const std::string& name() const
        {
            return this->nm;
        }

        const std::string& help() const
        {
            return this->hlp;
        }

        cmd_ptr function() const
        {
            return this->cmd;
        }
    };

    template <typename T>
    class parameters
    {
        std::vector<command<T>> cmds;

        void help(const std::map<std::string, command<T>>& cmds)
        {
            std::cout << "Help:" << std::endl;
            for (const auto& c : cmds)
            {
                std::cout << "\t" + c.second.name() << std::endl;
                std::cout << "\t\t" + c.second.help() << std::endl;
            }
        }

        std::string getline()
        {
            std::string line;
            std::getline(std::cin, line);
            return line;
        }

        std::string get_trimmed_line()
        {
            return trim(getline());
        }

        static constexpr auto chars =  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789"
                                "\\?'^\".:,;-_+/*=£$%&|()[]{}#@";

        static constexpr auto npos = std::string::npos;

        static std::string trim(const std::string& line)
        {
            auto b = line.find_first_of(chars);
            auto e = line.find_last_of(chars);
            // trim only if data availables
            return (b == npos) ? "" : line.substr(b, e-b+1);
        }

        static void parse(const std::string& trimmed, std::string& command, std::string& arguments)
        {
            using namespace std::literals;
            // search end of cmd:
            auto cb = trimmed.find_first_not_of(chars);
            auto cmd = (cb == npos) ? trimmed : trimmed.substr(0, cb);

            // search parameters
            auto pb = trimmed.find_first_of(chars, cmd.size());
            auto args = pb == npos ? (""s) : trimmed.substr(pb);

            command = std::move(cmd);
            arguments = std::move(args);
        }

        void handle_not_found(const std::string& command, const std::string& arguments)
        {
            (void)arguments;
            std::cerr << "Cmd: '" << command << "' not found\n";
        }

        std::string get_input()
        {
            std::string input;
            do
            {
                std::cout << prompt_str;
                input = get_trimmed_line();
            } while (input.empty());
            return input;
        }
    public:
        using cmd_ptr = typename command<T>::cmd_ptr;
        // Type used for handlers invocated before and after each command
        using before_cmd_ptr = void (T::*)(std::string&, std::string&);
        // Type used for handlers invocated before and after each command
        using after_cmd_ptr = void (T::*)(void);

        // Type used for handlers invocated before and after each command
        using before_all_ptr = void (T::*)(void);
        // Type used for handlers invocated before and after each command
        using after_all_ptr = void (T::*)(void);

        parameters() = default;
        parameters(const parameters&) = default;
        parameters(parameters&&) = default;

        auto& add_command(const std::string& name, cmd_ptr command, const std::string& help)
        {
            cmds.emplace_back(name, command, help);
            return *this;
        }

        const std::vector<command<T>>& commands() const
        {
            return this->cmds;
        }

        void repl(T& status_obj)
        {
            using namespace std::literals;

            std::map<std::string, command<T>> cmds;
            for (const auto& c : this->commands())
            {
                cmds[c.name()] = c;
            }

            if (this->b_all != nullptr)
            {
                try
                {
                    (status_obj.*b_all)();
                }
                catch(const std::exception& e)
                {
                    throw handler_error("before all handler threw", e);
                }
            }
            try
            {
                while (true)
                {
                    auto trimmed = get_input();
                    std::string cmd, args;
                    parse(trimmed, cmd, args);

                    if (cmd == "help"s) // requested help
                    {
                        help(cmds);
                        continue;
                    }

                    try
                    {
                        const auto& command = cmds.at(cmd);

                        if (this->before != nullptr)
                        {
                            try
                            {
                                (status_obj.*before)(cmd, args);
                            }
                            catch(const std::exception& e)
                            {
                                throw handler_error("before handler threw", e);
                            }
                        }
                        (status_obj.*command.function())(args);
                        if (this->after != nullptr)
                        {
                            try
                            {
                                (status_obj.*after)();
                            }
                            catch(const std::exception& e)
                            {
                                throw handler_error("after handler threw", e);
                            }
                        }
                    }
                    catch(const std::out_of_range&)
                    {
                        handle_not_found(cmd, args);
                        continue;
                    }
                }
            }
            catch(const exit& e) // normal termination
            {
                // to be called also at the end
                if (this->after != nullptr)
                {
                    try
                    {
                        (status_obj.*after)();
                    }
                    catch(const std::exception& e)
                    {
                        throw handler_error("after handler threw", e);
                    }
                }
                if (this->a_all != nullptr)
                {
                    try
                    {
                        (status_obj.*a_all)();
                    }
                    catch(const std::exception& e)
                    {
                        throw handler_error("after all handler threw", e);
                    }
                }
            }
        }

        auto& call_before_each_command(before_cmd_ptr handler)
        {
            this->before = handler;
            return *this;
        }

        auto& call_after_each_command(after_cmd_ptr handler)
        {
            this->after = handler;
            return *this;
        }

        auto& call_before_all(before_all_ptr handler)
        {
            this->b_all = handler;
            return *this;
        }

        auto& call_after_all(after_all_ptr handler)
        {
            this->a_all = handler;
            return *this;
        }

        auto& prompt(const std::string& prompt_str)
        {
            this->prompt_str = prompt_str;
            return *this;
        }

        const std::string& prompt() const
        {
            return this->prompt_str;
        }
    private:
        before_cmd_ptr before{};
        after_cmd_ptr after{};
        before_all_ptr b_all{};
        after_all_ptr a_all{};

        std::string prompt_str = ">>>";
    };

} // namespace repl

#endif
