#include "midi.h"

#include <Limelight.h>
#include <SDL.h>

MidiHandler::MidiHandler()
    : m_Started(false)
{
#ifdef Q_OS_WIN32
    memset(m_MidiInHandles, 0, sizeof(m_MidiInHandles));
    m_NumOpenDevices = 0;
#endif
}

MidiHandler::~MidiHandler()
{
    stop();
}

QVector<MidiDeviceInfo> MidiHandler::listInputDevices()
{
    QVector<MidiDeviceInfo> devices;

#ifdef Q_OS_WIN32
    UINT numDevs = midiInGetNumDevs();
    for (UINT i = 0; i < numDevs; i++) {
        MIDIINCAPSW caps;
        if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            MidiDeviceInfo info;
            info.id = (int)i;
            info.name = QString::fromWCharArray(caps.szPname);
            devices.append(info);
        }
    }
#endif

    return devices;
}

bool MidiHandler::start()
{
    if (m_Started) {
        return true;
    }

#ifdef Q_OS_WIN32
    UINT numDevs = midiInGetNumDevs();
    if (numDevs == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "No MIDI input devices found");
        return true;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Found %u MIDI input device(s)", numDevs);

    m_NumOpenDevices = 0;
    for (UINT i = 0; i < numDevs && m_NumOpenDevices < MAX_MIDI_INPUTS; i++) {
        MIDIINCAPSW caps;
        if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "MIDI input device %u: %ls", i, caps.szPname);
        }

        MMRESULT result = midiInOpen(&m_MidiInHandles[m_NumOpenDevices],
                                      i,
                                      (DWORD_PTR)midiInCallback,
                                      (DWORD_PTR)this,
                                      CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to open MIDI input device %u: error %u", i, result);
            continue;
        }

        result = midiInStart(m_MidiInHandles[m_NumOpenDevices]);
        if (result != MMSYSERR_NOERROR) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to start MIDI input device %u: error %u", i, result);
            midiInClose(m_MidiInHandles[m_NumOpenDevices]);
            m_MidiInHandles[m_NumOpenDevices] = NULL;
            continue;
        }

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Opened MIDI input device %u", i);
        m_NumOpenDevices++;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "MIDI handler started with %d device(s)", m_NumOpenDevices);
#endif

    m_Started = true;
    return true;
}

void MidiHandler::stop()
{
    if (!m_Started) {
        return;
    }

#ifdef Q_OS_WIN32
    for (int i = 0; i < m_NumOpenDevices; i++) {
        if (m_MidiInHandles[i] != NULL) {
            midiInStop(m_MidiInHandles[i]);
            midiInReset(m_MidiInHandles[i]);
            midiInClose(m_MidiInHandles[i]);
            m_MidiInHandles[i] = NULL;
        }
    }
    m_NumOpenDevices = 0;
#endif

    m_Started = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MIDI handler stopped");
}

#ifdef Q_OS_WIN32
void CALLBACK MidiHandler::midiInCallback(HMIDIIN hMidiIn, UINT wMsg,
                                            DWORD_PTR dwInstance,
                                            DWORD_PTR dwParam1,
                                            DWORD_PTR dwParam2)
{
    Q_UNUSED(hMidiIn);
    Q_UNUSED(dwInstance);
    Q_UNUSED(dwParam2);

    if (wMsg != MIM_DATA) {
        return;
    }

    // dwParam1 contains the MIDI message packed into a DWORD:
    // Low byte = status, next byte = data1, next byte = data2
    uint8_t data[3];
    data[0] = (uint8_t)(dwParam1 & 0xFF);         // Status byte
    data[1] = (uint8_t)((dwParam1 >> 8) & 0xFF);  // Data byte 1
    data[2] = (uint8_t)((dwParam1 >> 16) & 0xFF); // Data byte 2

    // Determine message length from status byte
    uint8_t status = data[0] & 0xF0;
    uint8_t length;
    switch (status) {
    case 0xC0: // Program Change
    case 0xD0: // Channel Pressure
        length = 2;
        break;
    case 0x80: // Note Off
    case 0x90: // Note On
    case 0xA0: // Polyphonic Aftertouch
    case 0xB0: // Control Change
    case 0xE0: // Pitch Bend
        length = 3;
        break;
    default:
        // System messages or unknown - send all 3 bytes
        length = 3;
        break;
    }

    LiSendMidiEvent(0, data, length);
}
#endif
