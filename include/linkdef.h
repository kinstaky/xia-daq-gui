#ifdef __CLING__
// Standard preamble: turn off creation of dictionaries for "everything":
// we then turn it on only for the types we are interested in.
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// // Turn on creation of dictionaries for nested classes
// #pragma link C++ nestedclasses;

// Turn on creation of dictionaries for class TerminationHandler
#pragma link C++ class TerminationHandler;

#endif // __CLING__