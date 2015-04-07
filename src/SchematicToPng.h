
// SchematicToPng.h

// Interfaces to the cSchematicToPng class encapsulating the entire app





#pragma once

#include <iostream>






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

		cQueueItem(const AString & a_InputFileName):
			m_InputFileName(a_InputFileName),
			m_OutputFileName(cFile::ChangeFileExt(a_InputFileName, "png")),
			m_StartX(-1),
			m_EndX(-1),
			m_StartY(-1),
			m_EndY(-1),
			m_StartZ(-1),
			m_EndZ(-1)
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
	
	/** List of threads that the server has running. */
	cThreadPtrs m_Threads;
	
	/** The number of threads that should be started. Configurable on the command line. */
	int m_NumThreads;
	
	/** If set to true, the chunk data is recompressed while saving each MCA file. */
	bool m_ShouldRecompress;
	
	
	/** Retrieves one item from the queue (and removes it from the queue).
	Returns nullptr when queue empty. */
	cQueueItemPtr GetNextQueueItem(void);

	/** Processes a file with the queue list into m_Queue. */
	void ProcessQueueFile(std::istream & a_File);

	/** Applies the property specified in a_PropertyLine to the specified queue item. */
	bool ProcessPropertyLine(cQueueItem & a_Item, const AString & a_PropertyLine);
} ;




