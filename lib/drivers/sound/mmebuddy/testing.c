/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Hax

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddsnd.h>
#include <debug.h>

#include <ntddk.h>
#include <mmebuddy.h>


/*
    **** TESTING CODE ONLY ****
*/

PSOUND_DEVICE Device;
PSOUND_DEVICE_INSTANCE Instance;
WAVEHDR waveheader;
WORD JunkBuffer[65536];


#define IDS_WAVEOUT_PNAME   0x68



BOOLEAN TestCallback(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle)
{
    MessageBox(0, DevicePath, L"CALLBACK", MB_OK | MB_TASKMODAL);

    AddSoundDevice(DeviceType, DevicePath);

    return TRUE;
}


APIENTRY VOID
Test()
{
    ULONG WaveInCount, WaveOutCount;
    ULONG MidiInCount, MidiOutCount;
    ULONG MixerCount, AuxCount;
    WCHAR Message[1024];

    DetectNt4SoundDevices(WAVE_IN_DEVICE_TYPE,
                          L"\\\\.\\SBWaveIn",
                          TestCallback);

    DetectNt4SoundDevices(WAVE_OUT_DEVICE_TYPE,
                          L"\\\\.\\SBWaveOut",
                          TestCallback);

    DetectNt4SoundDevices(MIDI_IN_DEVICE_TYPE,
                          L"\\\\.\\MidiIn",
                          TestCallback);

    DetectNt4SoundDevices(MIDI_OUT_DEVICE_TYPE,
                          L"\\\\.\\MidiOut",
                          TestCallback);

    DetectNt4SoundDevices(MIXER_DEVICE_TYPE,
                          L"\\\\.\\SBMixer",
                          TestCallback);

    DetectNt4SoundDevices(AUX_DEVICE_TYPE,
                          L"\\\\.\\SBAux",
                          TestCallback);

    WaveInCount = GetSoundDeviceCount(WAVE_IN_DEVICE_TYPE);
    WaveOutCount = GetSoundDeviceCount(WAVE_OUT_DEVICE_TYPE);
    MidiInCount = GetSoundDeviceCount(MIDI_IN_DEVICE_TYPE);
    MidiOutCount = GetSoundDeviceCount(MIDI_OUT_DEVICE_TYPE);
    MixerCount = GetSoundDeviceCount(MIXER_DEVICE_TYPE);
    AuxCount = GetSoundDeviceCount(AUX_DEVICE_TYPE);

    wsprintf(Message, L"Found devices:\n- %d wave inputs\n- %d wave outputs\n- %d midi inputs\n- %d midi outputs\n- %d mixers\n- %d aux devices",
        WaveInCount, WaveOutCount,
        MidiInCount, MidiOutCount,
        MixerCount, AuxCount);

    MessageBox(0, Message, L"Result", MB_OK | MB_TASKMODAL);
}

APIENTRY VOID
TestGetCaps()
{
    UNIVERSAL_CAPS Caps;
    WCHAR DevInfo[1024];
    PSOUND_DEVICE Device;
    MMRESULT Result;

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = GetSoundDeviceCapabilities(Device, &Caps);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    wsprintf(DevInfo, L"Device name: %hS\nManufacturer ID: %d\nProduct ID: %d\nDriver version: %x\nChannels: %d", Caps.WaveOut.szPname, Caps.WaveOut.wMid, Caps.WaveOut.wPid, Caps.WaveOut.vDriverVersion, Caps.WaveOut.wChannels);

    MessageBox(0, DevInfo, L"Device caps", MB_OK | MB_TASKMODAL);
}


VOID
TestFormatQuery()
{
    WCHAR msg[1024];
    PSOUND_DEVICE Device;
    MMRESULT Result;
    WAVEFORMATEX fmt;

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    /* Request a valid format */
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = 22050;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    Result = QueryWaveDeviceFormatSupport(Device, &fmt, sizeof(WAVEFORMATEX));

    wsprintf(msg, L"Format support query result: %d", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);

    /* Send it some garbage */
    fmt.nChannels = 6969;

    Result = QueryWaveDeviceFormatSupport(Device, &fmt, sizeof(WAVEFORMATEX));

    wsprintf(msg, L"Format support query result: %d", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);
}


VOID CALLBACK
OverlappedCallback(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    MessageBox(0, L"Job done!", L"File IO Callback", MB_OK | MB_TASKMODAL);
}


VOID
TestPlaybackHackingly()
{
    WCHAR msg[1024];
    MMRESULT Result;
    WAVEFORMATEX fmt;
    ULONG i;

    for ( i = 0; i < 65536; ++ i )
        JunkBuffer[i] = rand();

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);

    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = OpenKernelSoundDevice(Device, GENERIC_READ | GENERIC_WRITE);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail open", L"Fail", MB_OK | MB_TASKMODAL);
        return;
    }
    wsprintf(msg, L"Opened handle %x", Device->Handle);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);

    Result = CreateSoundDeviceInstance(Device, &Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail 2", MB_OK | MB_TASKMODAL);
        return;
    }

    /* Request a valid format */
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = 22050;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    Result = SetWaveDeviceFormat(Instance, &fmt, sizeof(WAVEFORMATEX));

    wsprintf(msg, L"Format support set result: %d\nClick to play!", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);

    //SOUND_DEBUG_HEX(Instance->Device->Handle);

    Result = StartWaveThread(Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Failed to start thread", L"Fail 3", MB_OK | MB_TASKMODAL);
        return;
    }

    //SOUND_DEBUG_HEX(Instance->Device->Handle);

    waveheader.lpData = (PVOID) JunkBuffer;
    waveheader.dwBufferLength = 65536;
    waveheader.dwFlags = WHDR_PREPARED;

    Result = QueueWaveDeviceBuffer(Instance, &waveheader);
//    CallSoundThread(Instance, WAVEREQUEST_QUEUE_BUFFER, &waveheader);
/*
    Result = WriteSoundDeviceBuffer(Instance,
                                    JunkBuffer, 65535, OverlappedCallback);
*/
    wsprintf(msg, L"Play result: %d", Result);
    MessageBox(0, msg, L"Result", MB_OK | MB_TASKMODAL);
}


APIENTRY VOID
TestDevEnum()
{
    EnumerateNt4ServiceSoundDevices(
        L"sndblst",
        WAVE_OUT_DEVICE_TYPE,
        TestCallback);
}


MMRESULT
TestThreadCallback(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  DWORD RequestId,
    IN  PVOID Data)
{
    MessageBox(0, L"Thread Request Callback", L"Woot", MB_OK | MB_TASKMODAL);

    return MMSYSERR_NOERROR;
}


WINAPI VOID
TestThreading()
{
    MMRESULT Result;
    PSOUND_DEVICE Device;
    PSOUND_DEVICE_INSTANCE Instance;

    AddSoundDevice(WAVE_OUT_DEVICE_TYPE, L"\\\\.\\SBWaveOut0");
    Result = GetSoundDevice(WAVE_OUT_DEVICE_TYPE, 0, &Device);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 1", L"Fail 1", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = CreateSoundDeviceInstance(Device, &Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 2", L"Fail 2", MB_OK | MB_TASKMODAL);
        return;
    }

    Result = StartWaveThread(Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        MessageBox(0, L"Fail 3", L"Fail 3", MB_OK | MB_TASKMODAL);
        return;
    }

    MessageBox(0, L"Click to send a request", L"Bai", MB_OK | MB_TASKMODAL);
    CallSoundThread(Instance, 69, NULL); 

    MessageBox(0, L"Click to kill thread", L"Bai", MB_OK | MB_TASKMODAL);

    StopWaveThread(Instance);
}


int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
//    TestFormatQuery();
    TestPlaybackHackingly();
//    TestDevEnum();
/*
    TestThreading();
*/
    MessageBox(0, L"Le end", L"Bai", MB_OK | MB_TASKMODAL);
    return 0;
}
