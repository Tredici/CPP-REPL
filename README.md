# CPP-REPL
C++ header only library to easily implement REPL applications.

# Compilation
No compilation is required. Just include header in jour source files and use it.

# Usage
Usage is simple:
- Define a class T to maintain application state.
- Instantiate an object repl::parameters<T> and configure it.
- Call method repl on the previous object passing an instance of T as arguments.

That'all! (Remind to throw repl::exit to end the application loop.)

# Examples
See main.cc for a working example.
