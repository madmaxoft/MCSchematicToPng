// JsonNet.h

// Interfaces to the json-over-net API





class cJsonNet
{
public:
	/** Starts the TCP server listening for Json API communication on the specified port.
	Returns true if successful, false otherwise. */
	static bool Start(UInt16 a_Port);
};




