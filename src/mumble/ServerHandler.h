// Copyright 2005-2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_SERVERHANDLER_H_
#define MUMBLE_MUMBLE_SERVERHANDLER_H_

#include <QtCore/QtGlobal>

#ifdef Q_OS_WIN
#	include "win.h"
#endif

#ifndef Q_MOC_RUN
#	include <boost/accumulators/accumulators.hpp>
#	include <boost/accumulators/statistics/mean.hpp>
#	include <boost/accumulators/statistics/stats.hpp>
#	include <boost/accumulators/statistics/variance.hpp>
#	include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QEvent>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCipher>
#include <QtNetwork/QSslError>

#define SERVERSEND_EVENT 3501

#include "Message.h"
#include "Mumble.pb.h"
#include "ServerAddress.h"
#include "Timer.h"

class Connection;
class Database;
class Message;
class PacketDataStream;
class QUdpSocket;
class QSslSocket;
class VoiceRecorder;

class ServerHandlerMessageEvent : public QEvent {
public:
	unsigned int uiType;
	QByteArray qbaMsg;
	bool bFlush;
	ServerHandlerMessageEvent(const QByteArray &msg, unsigned int type, bool flush = false);
};

typedef boost::shared_ptr< Connection > ConnectionPtr;

class ServerHandler : public QThread {
private:
	Q_OBJECT
	Q_DISABLE_COPY(ServerHandler)

	Database *database;

protected:
	QString qsHostName;
	QString qsUserName;
	QString qsPassword;
	unsigned short usPort;
	unsigned short usResolvedPort;
	bool bUdp;
	bool bStrong;

	/// Flag indicating whether the server we are currently connected to has
	/// finished synchronizing already.
	bool serverSynchronized = false;

#ifdef Q_OS_WIN
	HANDLE hQoS;
	DWORD dwFlowUDP;
#endif

	QHostAddress qhaRemote;
	QHostAddress qhaLocal;
	QUdpSocket *qusUdp;
	QMutex qmUdp;

	void handleVoicePacket(unsigned int msgFlags, PacketDataStream &pds, MessageHandler::UDPMessageType type);

	// kb
	qint64 iServerClientTimeDelta; // current best guess of microsecond difference between server clock & client clock (uses half estimated ping to correct for msg transmission time)
	quint64 uiServerTimeElapsed; // current guess as to server time...
	quint32 uiClientAudioLag; // estimated microseconds of lag introduced by client audio hardware (currently must be user-entered, later will be calculated)

	quint32 uiRiverOffset; // river offset of current room
	quint32 uiMediaIndex; // which song is playing
	quint32 uiRoleIndex; // which role of which song is playing
	quint64
		uiServerStartPerformanceTime; // server time in microseconds of start of performance, or zero for not performing
	quint64 uiPerformancePauseTime;   // performance time in microseconds of pause frame, or zero for not paused

	QMutex qmPerformanceLock;
	QMultiMap< float, QByteArray > qmPerformancePackets; // stores buffered packets for performances of all clients
	void addPerformanceFrame(float time, const QByteArray &packet);

public:
	quint32 getRiverOffset();
	void setMediaIndex(quint32 index);
	void setRoleIndex(quint32 index);
	quint32 getMediaIndex();        // get value 0..(numSongs-1) giving selected song
	quint32 getRoleIndex();        // 0=no voices, 1=A only, 2=B only, 3=Both
	quint64 getServerTimeElapsed(); // update and return current idea of elapsed time...
	void setKissyClientChannelState(quint32 riverOffset, quint64 serverStartPerformanceTime, quint64 performancePauseTime);
	void fetchPerformanceFrames();
	quint32 getAudioLag();
	void setAudioLag(quint32 audioLag);

	Timer tTimestamp;
	int iInFlightTCPPings;
	QTimer *tConnectionTimeoutTimer;
	QList< QSslError > qlErrors;
	QList< QSslCertificate > qscCert;
	QSslCipher qscCipher;
	ConnectionPtr cConnection;
	QByteArray qbaDigest;
	boost::shared_ptr< VoiceRecorder > recorder;
	QSslSocket *qtsSock;
	QList< ServerAddress > qlAddresses;
	ServerAddress saTargetServer;

	unsigned int uiVersion;
	QString qsRelease;
	QString qsOS;
	QString qsOSVersion;

	boost::accumulators::accumulator_set<
		double, boost::accumulators::stats< boost::accumulators::tag::mean, boost::accumulators::tag::variance,
											boost::accumulators::tag::count > >
		accTCP, accUDP, accClean;

	ServerHandler();
	~ServerHandler();
	void setConnectionInfo(const QString &host, unsigned short port, const QString &username, const QString &pw);
	void getConnectionInfo(QString &host, unsigned short &port, QString &username, QString &pw) const;
	bool isStrong() const;
	void customEvent(QEvent *evt) Q_DECL_OVERRIDE;

	void sendProtoMessage(const ::google::protobuf::Message &msg, unsigned int msgType);
	void sendMessage(const char *data, int len, bool force = false);

	/// @returns Whether this handler is currently connected to a server.
	bool isConnected() const;

	/// @returns Whether the server this handler is currently connected to, has finished
	/// 	synchronizing yet.
	bool hasSynchronized() const;

	/// @param synchronized Whether the server has finished synchronization
	void setServerSynchronized(bool synchronized);

#define MUMBLE_MH_MSG(x) \
	void sendMessage(const MumbleProto::x &msg) { sendProtoMessage(msg, MessageHandler::x); }
	MUMBLE_MH_ALL
#undef MUMBLE_MH_MSG

	void requestUserStats(unsigned int uiSession, bool statsOnly);
	void joinChannel(unsigned int uiSession, unsigned int channel);
	void joinChannel(unsigned int uiSession, unsigned int channel, const QStringList &temporaryAccessTokens);
	void startListeningToChannel(int channel);
	void startListeningToChannels(const QList< int > &channelIDs);
	void stopListeningToChannel(int channel);
	void stopListeningToChannels(const QList< int > &channelIDs);
	void createChannel(unsigned int parent_id, const QString &name, const QString &description, unsigned int position,
					   bool temporary, unsigned int maxUsers);
	void requestBanList();
	void requestUserList();
	void requestACL(unsigned int channel);
	void registerUser(unsigned int uiSession);
	void kickBanUser(unsigned int uiSession, const QString &reason, bool ban);
	void sendUserTextMessage(unsigned int uiSession, const QString &message_);
	void sendChannelTextMessage(unsigned int channel, const QString &message_, bool tree);
	void setUserComment(unsigned int uiSession, const QString &comment);
	void setUserTexture(unsigned int uiSession, const QByteArray &qba);
	void setTokens(const QStringList &tokens);
	void removeChannel(unsigned int channel);
	void addChannelLink(unsigned int channel, unsigned int link);
	void removeChannelLink(unsigned int channel, unsigned int link);
	void requestChannelPermissions(unsigned int channel);
	void setSelfMuteDeafState(bool mute, bool deaf);
	void announceRecordingState(bool recording);

	/// Return connection information as a URL
	QUrl getServerURL(bool withPassword = false) const;

	void disconnect();
	void run() Q_DECL_OVERRIDE;
signals:
	void error(QAbstractSocket::SocketError, QString reason);
	void disconnected(QAbstractSocket::SocketError, QString reason);
	void connected();
	void pingRequested();
protected slots:
	void message(unsigned int, const QByteArray &);
	void serverConnectionConnected();
	void serverConnectionTimeoutOnConnect();
	void serverConnectionStateChanged(QAbstractSocket::SocketState);
	void serverConnectionClosed(QAbstractSocket::SocketError, const QString &);
	void setSslErrors(const QList< QSslError > &);
	void udpReady();
	void hostnameResolved();
private slots:
	void sendPingInternal();
public slots:
	void sendPing();
};

typedef boost::shared_ptr< ServerHandler > ServerHandlerPtr;

#endif
