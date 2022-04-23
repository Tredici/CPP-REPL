#include <iostream>

#include "cpp_repl.hh"

using namespace std;

struct test
{
    void hello(const std::string&)
    {
        cout << "Hello!" << endl;
    }

    void display(const std::string& data)
    {
        cout << "You wrote: '" << data << "'" << endl;
    }

    void exit(const std::string&)
    {
        cout << "Exiting" << endl;
        throw repl::exit();
    }

    void before(std::string& cmd, std::string& args)
    {
        cout << "Calling '" << cmd << "' '" << args << "'\n";
    }
    void after()
    {
        cout << "Cmd executed!'\n";
    }
};

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;
    test t;
    repl::parameters<decltype(t)> pars;
    cout << "*** TEST REPL ***" << endl;

    pars.call_before_each_command(&test::before)
        .call_after_each_command(&test::after)
        .add_command("hello",   &test::hello,   "First example")
        .add_command("display", &test::display, "Print the given string")
        .add_command("exit",    &test::exit,    "End the loop");
    pars.repl(t);
    return 0;
}
