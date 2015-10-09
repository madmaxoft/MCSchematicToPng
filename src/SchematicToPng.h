
// SchematicToPng.h

// Interfaces to the cSchematicToPng class encapsulating the entire app





#pragma once

#include "Marker.h"
#include "InputStream.h"






class cSchematicToPng
{
public:
	
	cSchematicToPng(void);
	
	/** Reads the cmdline params and initializes the app.
	Returns true if the app should continue, false if not. */
	bool Init(int argc, char ** argv);
	
	/** Runs the entire app. */
	void Run(void);
	
protected:

	/** An item in the queue to be processed. */
	struct cQueueItem
	{
		AString m_InputFileName;
		AString m_OutputFileName;
		int m_StartX;
		int m_EndX;
		int m_StartY;
		int m_EndY;
		int m_StartZ;
		int m_EndZ;
		int m_HorzSize;
		int m_VertSize;
		int m_NumCCWRotations;
		cMarkerPtrs m_Markers;
		cInputStreamPtr m_ErrorOut;

		cQueueItem(const AString & a_InputFileName, cInputStreamPtr a_ErrorOut):
			m_InputFileName(a_InputFileName),
			m_OutputFileName(cFile::ChangeFileExt(a_InputFileName, "png")),
			m_StartX(-1),
			m_EndX(-1),
			m_StartY(-1),
			m_EndY(-1),
			m_StartZ(-1),
			m_EndZ(-1),
			m_HorzSize(4),
			m_VertSize(5),
			m_NumCCWRotations(0),
			m_ErrorOut(a_ErrorOut)
		{
		}
	};

	typedef std::shared_ptr<cQueueItem> cQueueItemPtr;
	typedef std::vector<cQueueItemPtr> cQueueItemPtrs;


	/** A single thread processing the schematic files from the queue */
	class cThread :
		public cIsThread
	{
		typedef cIsThread super;
		
	public:
		cThread(cSchematicToPng & a_Parent);
		
	protected:
		cSchematicToPng & m_Parent;
		
		
		/** Processes the specified item from the queue. */
		void ProcessItem(const cQueueItem & a_Item);
		
		// cIsThread overrides:
		virtual void Execute(void) override;
	} ;

	typedef std::shared_ptr<cThread> cThreadPtr;
	typedef std::vector<cThreadPtr> cThreadPtrs;


	/** The mutex protecting m_Queue agains multithreaded access. */
	cCriticalSection m_CS;

	/** The queue of schematic files to be processed by the threads. Protected by m_CS. */
	cQueueItemPtrs m_Queue;

	/** Event that is set each time a new item arrives into m_Queue. */
	cEvent m_evtQueue;

	/** List of threads that the server has running. */
	cThreadPtrs m_Threads;

	/** The number of threads that should be started. Configurable on the command line. */
	int m_NumThreads;

	/** The thread that accepts incoming connections in the network-daemon mode. */
	std::thread m_NetAcceptThread;

	/** Iff true, the main thread is kept running forever.
	Used for network-daemon mode. */
	bool m_KeepRunning;


	/** Retrieves one item from the queue (and removes it from the queue).
	Returns nullptr when queue empty. */
	cQueueItemPtr GetNextQueueItem(void);

	/** Processes a stream with the queue list into m_Queue. */
	void ProcessQueueStream(cInputStreamPtr a_Input);

	/** Applies the property specified in a_PropertyLine to the specified queue item.
	Returns true if successful, outputs message to stderr and returns false on error. */
	bool ProcessPropertyLine(cInputStreamPtr a_Input, cQueueItem & a_Item, const AString & a_PropertyLine);

	/** Adds a new marker, specified by text in a_MarkerValue, into a_Item.
	Returns true if successful, outputs message to stderr and returns false on error. */
	bool AddMarker(cQueueItem & a_Item, const AString & a_MarkerValue);

	/** Starts a server on the specified port that listens for connections and processes the text received on them as input file. */
	void StartNetServer(UInt16 a_Port);

	/** Accepts connections incoming on the specified socket and creates a new SocketInputThread for each such connection. */
	void NetAcceptThread(SOCKET s);

	/** Reads input from the socket and processes it as input file. */
	void NetInputThread(SOCKET s);
} ;




