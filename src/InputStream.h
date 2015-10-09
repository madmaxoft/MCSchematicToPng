
// InputStream.h

// Declares the cInputStream class and its descendants, used for parsing input for the program





#pragma once

#include <iostream>





/** Interface for the input parsing. */
class cInputStream
{
public:
	cInputStream(void);

	/** Reads one line of input, returns it in a_OutLine
	Returns true if the input was read successfully, false on EOF or other errors.
	Counts the number of lines read so far, so that it is available for error reporting. */
	bool GetLine(AString & a_OutLine);

	/** Called to display an error message as a reaction for an input line.
	Prepends the given error message with the current line number. */
	void LineError(const AString & a_ErrorMsg);

	/** Called to display an error message related to the input. */
	virtual void Error(const AString & a_ErrorMsg) = 0;

protected:
	/** Number of lines that have been read, for error reporting. */
	int m_LineNum;


	/** Reads one line of input, returns it in a_OutLine
	Returns true if the input was read successfully, false on EOF or other errors.
	This is the internal implementation that the descendants should override. */
	virtual bool GetLineInternal(AString & a_OutLine) = 0;
};

typedef std::shared_ptr<cInputStream> cInputStreamPtr;





/** Wraps the cInputStream interface around a std::istream and std::cerr */
class cIosInputStream:
	public cInputStream
{
public:
	cIosInputStream(std::istream & a_Input):
		m_Input(a_Input)
	{
	}

	// cInputStream overrides:
	virtual bool GetLineInternal(AString & a_OutLine) override;
	virtual void Error(const AString & a_ErrorMsg) override;

protected:
	std::istream & m_Input;
};




class cSocketInputStream:
	public cInputStream
{
public:
	cSocketInputStream(SOCKET a_Socket):
		m_Socket(a_Socket)
	{
	}

	// cInputStream overrides:
	virtual bool GetLineInternal(AString & a_OutLine) override;
	virtual void Error(const AString & a_ErrorMsg) override;

protected:
	SOCKET m_Socket;
};




