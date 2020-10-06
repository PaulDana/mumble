// Copyright 2005-2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_AUDIOOUTPUTSAMPLE_H_
#define MUMBLE_MUMBLE_AUDIOOUTPUTSAMPLE_H_

#include <QtCore/QFile>
#include <QtCore/QObject>
#include <sndfile.h>
#include <speex/speex_resampler.h>

#include "AudioOutputUser.h"

class SoundFile : public QObject {
private:
	Q_OBJECT
	Q_DISABLE_COPY(SoundFile)
protected:
	SNDFILE *sfFile;
	SF_INFO siInfo;
	QFile qfFile;
	static sf_count_t vio_get_filelen(void *user_data);
	static sf_count_t vio_seek(sf_count_t offset, int whence, void *user_data);
	static sf_count_t vio_read(void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_write(const void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_tell(void *user_data);

public:
	SoundFile(const QString &fname);
	~SoundFile();

	int channels() const;
	int samplerate() const;
	int error() const;
	QString strError() const;
	bool isOpen() const;

	sf_count_t seek(sf_count_t frames, int whence);
	sf_count_t read(float *ptr, sf_count_t items);

	// kb
	sf_count_t tell();
};

class AudioOutputSample : public AudioOutputUser {
private:
	Q_OBJECT
	Q_DISABLE_COPY(AudioOutputSample)
protected:
	unsigned int iLastConsume;
	unsigned int iBufferFilled;
	unsigned int iOutSampleRate;
	SpeexResamplerState *srs;

	SoundFile *sfHandle;

	bool bLoop;
	bool bEof;

	// kb
	bool bIsSynchronous; // if true this is Kissy style synchronous
	quint64 uiServerStartPerformanceTime; // if non zero we are playing (or pausing if pause time non zero)
	quint64 uiPerformancePauseTime;       // if non zero we are pausing (in which case the above start time must be set)
signals:
	void playbackFinished();

public:
	static SoundFile *loadSndfile(const QString &filename);
	static QString browseForSndfile(QString defaultpath = QString());
	// kb
	bool seekServerTime(quint64 serverTimeInMicroseconds, quint64 startTime);
	bool isSynchronous();
	quint64 getServerStartPerformanceTime();
	void setServerStartPerformanceTime(quint64 val);
	quint64 getPerformancePauseTime();
	void setPerformancePauseTime(quint64 val);

	virtual bool prepareSampleBuffer(unsigned int frameCount, quint64 serverTime, bool *doMix) Q_DECL_OVERRIDE;
	AudioOutputSample(const QString &name, SoundFile *psndfile, bool repeat, unsigned int freq,
					  unsigned int bufferSize, bool isSynchronous);
	~AudioOutputSample() Q_DECL_OVERRIDE;
};

#endif // AUDIOOUTPUTSAMPLE_H_
