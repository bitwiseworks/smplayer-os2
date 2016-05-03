/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2016 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "playerprocess.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#ifdef MPV_SUPPORT
#include "mpvprocess.h"
#endif

#ifdef MPLAYER_SUPPORT
#include "mplayerprocess.h"
#endif


PlayerProcess::PlayerProcess(QObject * parent) : MyProcess(parent) {
#if NOTIFY_SUB_CHANGES
	qRegisterMetaType<SubTracks>("SubTracks");
#endif

#if NOTIFY_AUDIO_CHANGES
	qRegisterMetaType<Tracks>("Tracks");
#endif

#if NOTIFY_CHAPTER_CHANGES
	qRegisterMetaType<Chapters>("Chapters");
#endif
}

void PlayerProcess::writeToStdin(QString text) {
	if (isRunning()) {
		qDebug("PlayerProcess::writeToStdin: %s", text.toUtf8().constData());
#if !defined(Q_OS_OS2)
		#ifdef Q_OS_WIN
		write( text.toUtf8() + "\n");
		#else
		write( text.toLocal8Bit() + "\n");
		#endif
#else
		PpipeWrite( text.toLocal8Bit() + "\n");
#endif
	} else {
		qWarning("PlayerProcess::writeToStdin: process not running");
	}
#ifdef Q_OS_OS2
	if (text == "quit" || text == "quit\n") {
		PpipeClose();
	}
#endif

}

PlayerProcess * PlayerProcess::createPlayerProcess(const QString & player_bin, QObject * parent) {
	PlayerProcess * proc = 0;

#if defined(MPV_SUPPORT) && defined(MPLAYER_SUPPORT)
	if (PlayerID::player(player_bin) == PlayerID::MPLAYER) {
		qDebug() << "PlayerProcess::createPlayerProcess: creating MplayerProcess";
		proc = new MplayerProcess(parent);
	} else {
		qDebug() << "PlayerProcess::createPlayerProcess: creating MPVProcess";
		proc = new MPVProcess(parent);
	}
#else
	#ifdef MPV_SUPPORT
	proc = new MPVProcess(parent);
	#endif
	#ifdef MPLAYER_SUPPORT
	proc = new MplayerProcess(parent);
	#endif
#endif

	return proc;
}

#ifdef CAPTURE_STREAM
void PlayerProcess::setCaptureDirectory(const QString & dir) {
	capture_filename = "";
	if (!dir.isEmpty() && (QFileInfo(dir).isDir())) {
		// Find a unique filename
		QString prefix = "capture";
		for (int n = 1; ; n++) {
			QString c = QDir::toNativeSeparators(QString("%1/%2_%3.dump").arg(dir).arg(prefix).arg(n, 4, 10, QChar('0')));
			if (!QFile::exists(c)) {
				capture_filename = c;
				return;
			}
		}
	}
}
#endif

#ifdef Q_OS_OS2
void PlayerProcess::PpipeOpen(const char *szPipeName) {
	DosCreateNPipe( szPipeName, &hpipe, NP_ACCESS_DUPLEX, 1, 1024, 1024, 0 );
}

void PlayerProcess::PpipeClose( void ) {
	DosClose( hpipe );
}

void PlayerProcess::PpipeWrite( const QByteArray text ) {
	// MPlayer quitted ?
	if ( !isRunning() )
		return;

// as things could hang when pipe communication is done direct here, i do a seperate thread for it
	pipeThread = new PipeThread(text, hpipe);
    
	pipeThread->start();
	while (!pipeThread->isRunning() && !pipeThread->isFinished()) {
		qDebug("we sleep");
		DosSleep(10);
	}
// we wait for max 2 seconds for the thread to be ended (we to this with max 20 loops)
	int count = 0;
	while (!pipeThread->wait(100) && count < 20) {
		count ++;
	}
	if (count >= 20) {
		pipeThread->terminate();
		qDebug("pipe communication terminated");
	}
	delete pipeThread;
}

PipeThread::PipeThread(const QByteArray t, const HPIPE pipe) {
	text = t;
	hpipe = pipe;
}

PipeThread::~PipeThread() {
}

void PipeThread::run() {
	ULONG cbActual;
	APIRET rc = NO_ERROR; 

	rc = DosConnectNPipe( hpipe );
	if (rc != NO_ERROR) 
		return;

//	qDebug("pipe connected");    
	DosWrite( hpipe, text.data(), strlen( text.data() ), &cbActual );

	// Wait for ACK
	DosRead( hpipe, &cbActual, sizeof( ULONG ), &cbActual );
	DosDisConnectNPipe( hpipe );
	return;
}
#endif

#include "moc_playerprocess.cpp"
