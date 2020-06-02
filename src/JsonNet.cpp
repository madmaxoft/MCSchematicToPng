// JsonNet.cpp

// Implements the json-over-network API

/*
The protocol:
Each side sends json-formatted messages, separated by ETB character (0x17).
The client includes a CmdID member in the json that the server includes verbatim in the response.
Upon connection, the server first sends a json message containing its version.
*/

#include "Globals.h"
#include "JsonNet.h"
#include <sstream>
#include <thread>
#include <functional>
#include "json/json.h"
#include "WorldStorage/FastNBT.h"
#include "StringCompression.h"
#include "BlockImage.h"
#include "PngExporter.h"
#include "Marker.h"





#if defined(_WIN32)
	#define NET_LAST_ERROR WSAGetLastError()
#else
	#define NET_LAST_ERROR errno
#endif





class cJsonNetConnection
{
public:
	cJsonNetConnection(SOCKET a_Socket):
		m_Socket(a_Socket)
	{
		// Initialize the remote peer information in m_ClientIPPort
		sockaddr address;
		socklen_t addressLen = sizeof(address);
		if (getpeername(a_Socket, &address, &addressLen) == 0)
		{
			char addressStr[NI_MAXHOST];
			char portStr[NI_MAXSERV];
			if (getnameinfo(&address, addressLen, addressStr, sizeof(addressStr), portStr, sizeof(portStr), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
			{
				m_ClientIPPort = Printf("%s:%s", addressStr, portStr);
			}
			else
			{
				m_ClientIPPort = "<unknown address>";
			}
		}
		m_Identification = m_ClientIPPort;

		// Start the connection thread:
		m_Thread = std::thread(std::bind(&cJsonNetConnection::Run, this));
		m_Thread.detach();
	}

protected:
	/** The OS socket object for this connection. */
	SOCKET m_Socket;

	/** Identification of the client that is used for logging.
	Usually contains the remote peer IP address and port; can be changed by the protocol. */
	AString m_Identification;

	/** The IP and port of the remote peer. Used for logging. */
	AString m_ClientIPPort;

	/** The thread which handles the connection. */
	std::thread m_Thread;

	/** The value of cmdID of the currently processed command.
	The client sends any value as the cmdID, we return it in the response for that command,
	so that the client can pair the command with the response. */
	Json::Value m_CurrentCmdID;



	/** Handles the connection's lifetime - reads requests and writes responses to the socket.
	When the connection is closed, deletes self. */
	void Run(void)
	{
		Json::Value welcomeMsg;
		welcomeMsg["MCSchematicToPng"] = 2;
		SendResponse(welcomeMsg);
		ProcessIncomingData();
		delete this;
	}



	/** Reads the incoming data.
	When a full command is received, it is executed and response is written back to the socket. */
	void ProcessIncomingData()
	{
		// Read from the socket until an ETB character (0x17), then process as JSON:
		AString req;
		for (;;)
		{
			char buf[128];
			int numReceived = static_cast<int>(recv(m_Socket, buf, sizeof(buf), 0));
			if (numReceived < 0)
			{
				LOGWARNING("Socket %s received an error %d, LastError = %d. Closing connection.", m_Identification.c_str(), numReceived, NET_LAST_ERROR);
				closesocket(m_Socket);
				return;
			}
			if (numReceived == 0)
			{
				LOG("Socket %s closed.", m_Identification.c_str());
				return;
			}
			int start = 0;
			for (int i = 0; i < numReceived; i++)
			{
				if (buf[i] == 0x17)
				{
					auto numBytes = i - start;
					if (numBytes > 0)
					{
						req.append(buf + start, static_cast<size_t>(numBytes));
					}
					if (!ProcessReq(req))
					{
						LOGWARNING("Failed to process request on socket %s, closing the socket.", m_Identification.c_str());
						closesocket(m_Socket);
						return;
					}
					req.clear();
					start = i + 1;
				}
			}  // for i - buf[]
			if (start < numReceived)
			{
				auto numBytes = numReceived - start;
				req.append(buf + start, numBytes);
			}
		}  // for (-ever)
	}



	/** Processes a single Json request incoming on the socket.
	Returns true if successful, false on error. */
	bool ProcessReq(const AString & a_Request)
	{
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		Json::Value req;
		JSONCPP_STRING err;
		std::istringstream is(a_Request);
		bool ok = parseFromStream(builder, is, &req, &err);
		if (!ok)
		{
			LOGWARNING("Error while parsing json on socket %s: %s", m_Identification.c_str(), err.c_str());
			return false;
		}
		return ProcessReq(req);
	}




	/** Processes a single Json request incoming on the socket.
	Returns true if successful, false on error. */
	bool ProcessReq(const Json::Value & a_Request)
	{
		auto cmd = a_Request["Cmd"].asString();
		m_CurrentCmdID = a_Request["CmdID"];
		if (cmd == "RenderSchematic")
		{
			return ProcessRenderSchematic(a_Request);
		}
		else if (cmd == "SetName")
		{
			auto name = a_Request["Name"].asString();
			if (!name.empty())
			{
				m_Identification = Printf("%s (%s)", name.c_str(), m_ClientIPPort.c_str());
			}
			return true;
		}
		else
		{
			LOGWARNING("Error in json on socket %s: missing or invalid cmd (\"%s\")", m_Identification.c_str(), cmd.c_str());
			return false;
		}
		return true;
	}



	/** Processes a RenderSchematic cmd incoming on the socket.
	Returns true if successful, false on error. */
	bool ProcessRenderSchematic(const Json::Value & a_Request)
	{
		try
		{
			auto blockData = a_Request.get("BlockData", "").asString();

			auto unBase64ed = Base64Decode(blockData);
			AString contents;
			if (UncompressStringGZIP(unBase64ed.data(), unBase64ed.size(), contents) != Z_OK)
			{
				SendSimpleError("Failed to decompress block data.");
				return true;
			}
			cParsedNBT nbt(contents.data(), contents.size());
			if (!nbt.IsValid())
			{
				SendSimpleError("Cannot NBT-parse input file.");
				return true;
			}
			int tHeight = nbt.FindChildByName(0, "Height");
			int tLength = nbt.FindChildByName(0, "Length");
			int tWidth  = nbt.FindChildByName(0, "Width");
			if (
				(tHeight <= 0) || (tLength <= 0) || (tWidth <= 0) ||
				(nbt.GetType(tHeight) != TAG_Short) ||
				(nbt.GetType(tLength) != TAG_Short) ||
				(nbt.GetType(tWidth) != TAG_Short)
			)
			{
				SendSimpleError("NBT data doesn't contain dimensions!");
				return true;
			}
			int height = nbt.GetShort(tHeight);
			int length = nbt.GetShort(tLength);
			int width  = nbt.GetShort(tWidth);

			// Get the dimensions from the request, combine with actual dimensions:
			auto startX = Clamp(a_Request.get("StartX", 0).asInt(), 0, length);
			auto endX   = Clamp(a_Request.get("EndX"  , width - 1).asInt(), 0, width);
			auto startY = Clamp(a_Request.get("StartY", 0).asInt(), 0, height);
			auto endY   = Clamp(a_Request.get("EndY", height - 1).asInt(), 0, height);
			auto startZ = Clamp(a_Request.get("StartZ", 0).asInt(), 0, length);
			auto endZ   = Clamp(a_Request.get("EndZ", length - 1).asInt(), 0, length);
			if ((endX - startX < 0) || (endY - startY < 0) || (endZ - startZ < 0))
			{
				SendSimpleError(Printf("The specified dimensions result in an empty area ({%d, %d, %d}).",
					endX - startX, endY - startY, endZ - startZ
				));
				return true;
			}

			// Get the pointers to block data in the NBT:
			int tBlocks = nbt.FindChildByName(0, "Blocks");
			int tMetas = nbt.FindChildByName(0, "Data");
			if (
				(tBlocks <= 0) || (tMetas <= 0) ||
				(nbt.GetType(tBlocks) != TAG_ByteArray) ||
				(nbt.GetType(tMetas)  != TAG_ByteArray)
			)
			{
				SendSimpleError("NBT data doesn't contain block data or meta data!");
				return true;
			}
			auto blocks = reinterpret_cast<const Byte *>(nbt.GetData(tBlocks));
			auto metas  = reinterpret_cast<const Byte *>(nbt.GetData(tMetas));

			// Parse the markers:
			auto markers = a_Request["Markers"];
			cMarkerPtrs imgMarkers;
			for (const auto & marker: markers)
			{
				auto shapeStr = marker["Shape"].asString();
				auto shape = cMarkerShape::GetShapeForName(shapeStr);
				if (shape == nullptr)
				{
					SendSimpleError(Printf("Unknown marker shape: \"%s\".", shapeStr.c_str()));
					return false;
				}
				int color;
				if (!HexStringToInteger(marker["Color"].asString(), color))
				{
					SendSimpleError(Printf("Invalid marker color specification: \"%s\".", marker["Color"].asCString()));
					return false;
				}
				imgMarkers.push_back(std::make_shared<cMarker>(marker["X"].asInt(), marker["Y"].asInt(), marker["Z"].asInt(), shape, color));
			}

			// Copy the block data out of the NBT:
			auto sizeX = endX - startX + 1;
			auto sizeY = endY - startY + 1;
			auto sizeZ = endZ - startZ + 1;
			cBlockImage Img(sizeX, sizeY, sizeZ);
			for (int y = 0; y < sizeY; y++)
			{
				for (int z = 0; z < sizeZ; z++)
				{
					for (int x = 0; x < sizeX; x++)
					{
						int idx = startX + x + (startZ + z) * width + (startY + y) * width * length;
						Byte BlockType = blocks[idx];
						Byte BlockMeta = metas[idx] & 0x0f;
						Img.SetBlock(x, y, z, BlockType, BlockMeta);
					}
				}
			}

			// Apply the rotations:
			auto numCWRotations = a_Request.get("NumCWRotations", 0).asInt();
			auto numCCWRotations = (4 - (numCWRotations % 4)) % 4;
			for (int i = 0; i < numCCWRotations; i++)
			{
				Img.RotateCCW();
			}

			// Export as PNG image:
			auto horzSize = a_Request.get("HorzSize", 4).asInt();
			auto vertSize = a_Request.get("VertSize", 5).asInt();
			auto dataOut = cPngExporter::Export(Img, horzSize, vertSize, imgMarkers);

			// Send the response:
			Json::Value resp;
			resp["Status"] = "ok";
			resp["CmdID"] = m_CurrentCmdID;
			resp["PngData"] = Base64Encode(dataOut);
			SendResponse(resp);
		}
		catch (const std::exception & exc)
		{
			LOGWARNING("Error in RenderSchematic command on socket %s: %s", m_Identification.c_str(), exc.what());
			return false;
		}
		return true;
	}



	/** Sends an error response. */
	void SendSimpleError(const AString & a_Error)
	{
		Json::Value resp;
		resp["CmdID"] = m_CurrentCmdID;
		resp["Status"] = "error";
		resp["ErrorText"] = a_Error;
		SendResponse(resp);
	}




	/** Sends the json as a response. */
	void SendResponse(const Json::Value & a_Response)
	{
		auto toSend = a_Response.toStyledString();
		send(m_Socket, toSend.data(), toSend.size(), 0);
		char msgEnd = 0x17;
		send(m_Socket, &msgEnd, 1, 0);
	}
};





class cJsonNetServer
{
public:
	cJsonNetServer(SOCKET a_Socket)
	{
		m_NetAcceptThread = std::thread(std::bind(&cJsonNetServer::NetAcceptThread, this, a_Socket));
	}

protected:
	/** Thread that accepts incoming connections and creates a separate handler thread for each. */
	std::thread m_NetAcceptThread;




	/** Accepts connections incoming on the specified socket and creates a new SocketInputThread for each such connection. */
	void NetAcceptThread(SOCKET a_Socket)
	{
		SOCKET s;
		sockaddr_storage sa;
		socklen_t salen = static_cast<socklen_t>(sizeof(sa));
		while ((s = accept(a_Socket, reinterpret_cast<sockaddr *>(&sa), &salen)) != INVALID_SOCKET)
		{
			LOG("Accepted a new json network connection: %d", s);

			// Create a new thread that parses queue items out from the socket:
			new cJsonNetConnection(s);
		}
	}
};





////////////////////////////////////////////////////////////////////////////////
// cJsonNet:

bool cJsonNet::Start(UInt16 a_Port)
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		LOG("Cannot create socket on port %u for json-net-api", a_Port);
		return false;
	}
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(a_Port);
	if (bind(s, reinterpret_cast<const sockaddr *>(&sa), sizeof(sa)) != 0)
	{
		LOG("Cannot bind socket to port %u for json-net-api", a_Port);
		return false;
	}
	if (listen(s, 0) != 0)
	{
		LOG("Cannot listen on port %u for json-net-api", a_Port);
		return false;
	}
	new cJsonNetServer(s);  // Leak this pointer on purpose - continue living until app termination
	LOG("Port %u is open for incoming json-net-api connections.", a_Port);
	return true;
}




