#include <Novice.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <mmsystem.h>
#include <string>
#include <iostream>
#include <ImGuiManager.h>

IXAudio2* pXAudio2 = nullptr;
IXAudio2MasteringVoice* pMasteringVoice = nullptr;
IXAudio2SourceVoice* pSourceVoice = nullptr;
IXAudio2SubmixVoice* pSubmixVoice = nullptr;

struct WaveData {
    WAVEFORMATEX m_wavFormat;
    char* m_soundBuffer;
    DWORD m_size;

    WaveData() : m_soundBuffer(nullptr), m_size(0) {}
    ~WaveData() { Release(); }

    void Release() {
        if (m_soundBuffer) {
            delete[] m_soundBuffer;
            m_soundBuffer = nullptr;
        }
        m_size = 0;
    }
};

WaveData waveData;

float reflectionsDelay = 0.0f;
float reverbDelay = 0.0f;
float roomSize = -10000.0f;
float density = 0.0f;
bool isReverb = false;
bool prevIsReverb = false;


bool LoadWaveFile(const std::wstring& wFilePath, WaveData* outData);
bool PlayWaveSound(const std::wstring& fileName, WaveData* outData, bool loop);
bool InitializeXAudio2();
void ReleaseXAudio2Resources();
void CleanupEffect();
bool AddReverbEffect(float reflectionsDelay, float reverbDelay, float roomSize, float density);
void RemoveReverbEffect();
bool IsReverbEffectApplied();
bool SetReverbParameters(float reflectionsDelay, float reverbDelay, float roomSize, float density);

const char kWindowTitle[] = "LC1B_10_カワグチ_ハルキ_タイトル";

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // XAudio2の初期化
    if (!InitializeXAudio2()) {
        std::cerr << "Failed to initialize XAudio2." << std::endl;
        ReleaseXAudio2Resources();
        return 1;
    }

    // Play WAV file
    if (!PlayWaveSound(L"clapSound.wav", &waveData, true)) {
        std::cerr << "Failed to play sound." << std::endl;
        ReleaseXAudio2Resources();
        return 1;
    }

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        // ImGuiの更新処理内でパラメータを設定する
  /*      if (isReverb) {
            if (!IsReverbEffectApplied()) {
                if (!AddReverbEffect(reflectionsDelay, reverbDelay, roomSize, density)) {
                    std::cerr << "Failed to add reverb effect." << std::endl;
                }
            }
            else {
                if (!SetReverbParameters(reflectionsDelay, reverbDelay, roomSize, density)) {
                    std::cerr << "Failed to set reverb parameters." << std::endl;
                }
            }
        }
        else {
            if (IsReverbEffectApplied()) {
                RemoveReverbEffect();
            }
        }*/
        
        if (isReverb != prevIsReverb) {
            // フラグが切り替わった場合、現在の音声を停止
            if (pSourceVoice) {
                pSourceVoice->Stop();
                pSourceVoice->FlushSourceBuffers();
                pSourceVoice->DestroyVoice();
                pSourceVoice = nullptr;
            }

            // 新しい音声を再生
            if (isReverb) {
                if (!PlayWaveSound(L"clapSoundEcho.wav", &waveData, true)) {
                    std::cerr << "Failed to play sound." << std::endl;
                    ReleaseXAudio2Resources();
                    return 1;
                }
            }
            else {
                if (!PlayWaveSound(L"clapSound.wav", &waveData, true)) {
                    std::cerr << "Failed to play sound." << std::endl;
                    ReleaseXAudio2Resources();
                    return 1;
                }
            }
        }

        // フラグの状態を更新
        prevIsReverb = isReverb;


        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        // ImGui window for reverb parameters
        ImGui::Begin("Reverb Parameters");
        //ImGui::SliderFloat("Reflections Delay", &reflectionsDelay, 0.0f, 0.3f);
        //ImGui::SliderFloat("Reverb Delay", &reverbDelay, 0.0f, 0.1f);
        //ImGui::SliderFloat("Room Size", &roomSize, -1000.0f, 0.0f);
        //ImGui::SliderFloat("Density", &density, 0.0f, 100.0f);
        ImGui::Checkbox("isReverb", &isReverb);
        ImGui::End();

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    ReleaseXAudio2Resources();

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}

bool InitializeXAudio2() {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        std::cerr << "Failed to initialize COM." << std::endl;
        return false;
    }

    result = XAudio2Create(&pXAudio2);
    if (FAILED(result)) {
        std::cerr << "Failed to create XAudio2 engine." << std::endl;
        CoUninitialize();
        return false;
    }

    result = pXAudio2->CreateMasteringVoice(&pMasteringVoice);
    if (FAILED(result)) {
        std::cerr << "Failed to create mastering voice." << std::endl;
        pXAudio2->Release();
        CoUninitialize();
        return false;
    }

    return true;
}

void ReleaseXAudio2Resources() {
    if (pSourceVoice) {
        pSourceVoice->DestroyVoice();
        pSourceVoice = nullptr;
    }
    if (pSubmixVoice) {
        pSubmixVoice->DestroyVoice();
        pSubmixVoice = nullptr;
    }
    if (pMasteringVoice) {
        pMasteringVoice->DestroyVoice();
        pMasteringVoice = nullptr;
    }
    if (pXAudio2) {
        pXAudio2->Release();
        pXAudio2 = nullptr;
        CoUninitialize();
    }
}

void CleanupEffect() {
    // Add any specific cleanup code here if needed
}

bool AddReverbEffect(float ReflectionsDelay, float ReverbDelay, float RoomSize, float Density) {
    HRESULT hr;
    XAUDIO2_EFFECT_DESCRIPTOR effects[1] = {};
    IUnknown* pReverbEffect = nullptr;

    // Create reverb effect
    hr = XAudio2CreateReverb(&pReverbEffect);
    if (FAILED(hr)) {
        std::cerr << "Failed to create reverb effect." << std::endl;
        return false;
    }

    // Set up effect descriptor
    effects[0].pEffect = pReverbEffect;
    effects[0].InitialState = TRUE;
    effects[0].OutputChannels = 1; // Number of output channels for reverb effect

    // Create effect chain
    XAUDIO2_EFFECT_CHAIN effectChain = {};
    effectChain.EffectCount = 1;
    effectChain.pEffectDescriptors = effects;

    // Create submix voice
    hr = pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1, 44100, 0, 0, nullptr, &effectChain);
    if (FAILED(hr)) {
        std::cerr << "Failed to create submix voice." << std::endl;
        pReverbEffect->Release();
        return false;
    }

    // Set reverb effect parameters
    XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverbParams = {};
    reverbParams.WetDryMix = 100;

    reverbParams.ReflectionsDelay = ReflectionsDelay;
    reverbParams.ReverbDelay = ReverbDelay;
    reverbParams.Room = (int)RoomSize;
    reverbParams.Density = Density;

    hr = pSubmixVoice->SetEffectParameters(0, &reverbParams, sizeof(reverbParams));
    if (FAILED(hr)) {
        std::cerr << "Failed to set reverb parameters." << std::endl;
        pSubmixVoice->DestroyVoice();
        pReverbEffect->Release();
        return false;
    }

    // Release reverb effect (submix voice holds a reference)
    pReverbEffect->Release();
    return true;
}

void RemoveReverbEffect() {
    if (pSubmixVoice) {
        // Destroy the submix voice
        pSubmixVoice->DestroyVoice();
        pSubmixVoice = nullptr;
    }

    // Optionally release any resources or additional cleanup here
}

bool IsReverbEffectApplied() {
    return (pSubmixVoice != nullptr);
}

bool SetReverbParameters(float ReflectionsDelay, float ReverbDelay, float RoomSize, float Density) {
    if (!IsReverbEffectApplied()) {
        return false;
    }

    HRESULT hr;
    XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverbParams = {};
    reverbParams.WetDryMix = 100;

    reverbParams.ReflectionsDelay = ReflectionsDelay;
    reverbParams.ReverbDelay = ReverbDelay;
    reverbParams.Room = (int)RoomSize;
    reverbParams.Density = Density;

    hr = pSubmixVoice->SetEffectParameters(0, &reverbParams, sizeof(reverbParams));
    if (FAILED(hr)) {
        std::cerr << "Failed to set reverb parameters." << std::endl;
        return false;
    }

    return true;
}

bool LoadWaveFile(const std::wstring& wFilePath, WaveData* outData) {
    MMCKINFO parentChunk;
    MMCKINFO childChunk;
    WAVEFORMATEX waveFormat;
    HRESULT hr;
    HMMIO hWavFile;

    hWavFile = mmioOpen(const_cast<LPWSTR>(wFilePath.c_str()), nullptr, MMIO_ALLOCBUF | MMIO_READ);
    if (hWavFile == nullptr) {
        std::cerr << "Failed to open wave file." << std::endl;
        return false;
    }

    parentChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    hr = mmioDescend(hWavFile, &parentChunk, nullptr, MMIO_FINDRIFF);
    if (FAILED(hr)) {
        std::cerr << "Failed to descend into WAVE chunk." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    childChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    hr = mmioDescend(hWavFile, &childChunk, &parentChunk, MMIO_FINDCHUNK);
    if (FAILED(hr)) {
        std::cerr << "Failed to descend into fmt chunk." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    if (mmioRead(hWavFile, reinterpret_cast<HPSTR>(&waveFormat), sizeof(waveFormat)) != sizeof(waveFormat)) {
        std::cerr << "Failed to read wave format." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    if (waveFormat.wFormatTag != WAVE_FORMAT_PCM) {
        std::cerr << "Unsupported wave format." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    hr = mmioAscend(hWavFile, &childChunk, 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to ascend fmt chunk." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    childChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    hr = mmioDescend(hWavFile, &childChunk, &parentChunk, MMIO_FINDCHUNK);
    if (FAILED(hr)) {
        std::cerr << "Failed to descend into data chunk." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    outData->m_size = childChunk.cksize;
    outData->m_soundBuffer = new char[outData->m_size];
    if (mmioRead(hWavFile, reinterpret_cast<HPSTR>(outData->m_soundBuffer), outData->m_size) != static_cast<LONG>(outData->m_size)) {
        std::cerr << "Failed to read wave data." << std::endl;
        mmioClose(hWavFile, 0);
        return false;
    }

    outData->m_wavFormat = waveFormat;

    mmioClose(hWavFile, 0);
    return true;
}

bool PlayWaveSound(const std::wstring& fileName, WaveData* outData, bool loop) {
    HRESULT hr;

    // Load wave file
    if (!LoadWaveFile(fileName, outData)) {
        std::cerr << "Failed to load wave file." << std::endl;
        return false;
    }

    // Create source voice
    hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &outData->m_wavFormat);
    if (FAILED(hr)) {
        std::cerr << "Failed to create source voice." << std::endl;
        return false;
    }

    // Submit audio data
    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = outData->m_size;
    buffer.pAudioData = reinterpret_cast<BYTE*>(outData->m_soundBuffer);
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to submit source buffer." << std::endl;
        return false;
    }

    // Start audio playback
    hr = pSourceVoice->Start(0);
    if (FAILED(hr)) {
        std::cerr << "Failed to start source voice." << std::endl;
        return false;
    }

    return true;
}
