// files within this DLL are compiled with the GENERICGPIB_EXPORTS
// defined with this macro as being exported.
#define GENERICGPIB_EXPORTS
#ifdef GENERICGPIB_EXPORTS
#define HANDLER_API __declspec(dllexport)
#else
#define HANDLER_API __declspec(dllimport)
#endif




