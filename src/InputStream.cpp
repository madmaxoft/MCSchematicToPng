
// InputStream.cpp

// Implements the cInputStream class and its descendants, used for parsing input for the program

#include "Globals.h"
#include "InputStream.h"





////////////////////////////////////////////////////////////////////////////////
// cInputStream:

cInputStream::cInputStream(void):
	m_LineNum(0)
{
}





bool cInputStream::GetLine(AString & a_OutLine)
{
	if (!GetLineInternal(a_OutLine))
	{
		return false;
	}
	m_LineNum += 1;
	return true;
}





void cInputStream::LineError(const AString & a_ErrorMsg)
{
	Error(Printf("Error while parsing line %d: %s", m_LineNum, a_ErrorMsg.c_str()));
}





////////////////////////////////////////////////////////////////////////////////
// cIosInputStream:

bool cIosInputStream::GetLineInternal(AString & a_OutLine)
{
	if (!m_Input.good())
	{
		return false;
	}
	std::getline(m_Input, a_OutLine);
	return true;
}





void cIosInputStream::Error(const AString & a_ErrorMsg)
{
	std::cerr << a_ErrorMsg << std::endl;
}





////////////////////////////////////////////////////////////////////////////////
// cSocketInputStream:

bool cSocketInputStream::GetLineInternal(AString & a_OutLine)
{
	a_OutLine.clear();
	for (;;)
	{
		char c;
		if (recv(m_Socket, &c, 1, 0) != 1)
		{
			return false;
		}
		if (c == 10)
		{
			return true;
		}
		else if (c != 13)
		{
			a_OutLine.push_back(c);
		}
	}
}





void cSocketInputStream::Error(const AString & a_ErrorMsg)
{
	send(m_Socket, a_ErrorMsg.c_str(), static_cast<int>(a_ErrorMsg.length()), 0);
	static const char EndLine[] = "\n";
	send(m_Socket, EndLine, sizeof(EndLine) - 1, 0);

}




