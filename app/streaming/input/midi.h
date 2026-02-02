#pragma once

#include <QObject>
#include <QVector>
#include <QString>

#ifdef Q_OS_WIN32
#include <Windows.h>
#include <mmsystem.h>
#endif

struct MidiDeviceInfo {
    int id;
    QString name;
};

class MidiHandler
{
public:
    MidiHandler();
    ~MidiHandler();

    bool start();
    void stop();

    QVector<MidiDeviceInfo> listInputDevices();

private:
#ifdef Q_OS_WIN32
    static void CALLBACK midiInCallback(HMIDIIN hMidiIn, UINT wMsg,
                                         DWORD_PTR dwInstance,
                                         DWORD_PTR dwParam1,
                                         DWORD_PTR dwParam2);

    static const int MAX_MIDI_INPUTS = 16;
    HMIDIIN m_MidiInHandles[MAX_MIDI_INPUTS];
    int m_NumOpenDevices;
#endif

    bool m_Started;
};
