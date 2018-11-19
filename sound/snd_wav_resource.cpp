/*=========================================================
Name    :   snd_wav_resource.cpp
Date    :   10/14/2016
=========================================================*/

#include "snd_main.h"
#include "snd_wav_resource.h"
#include "../system/resource.h"

/*=========================================================
=========================================================*/

static const struct { int iResource; char const* szResource; } resources[] =
{
    { IDR_WAVE1, "assets/sound/cannon_impact.wav" },
    { IDR_WAVE2, "assets/sound/tank_explode.wav" },
    { IDR_WAVE3, "assets/sound/cannon_fire.wav" },
    { IDR_WAVE4, "assets/sound/tank_idle.wav" },
    { IDR_WAVE5, "assets/sound/tank_move.wav" },
    { IDR_WAVE6, "assets/sound/turret_move.wav" },
    { IDR_WAVE7, "assets/sound/blaster_fire.wav" },
    { IDR_WAVE8, "assets/sound/blaster_impact.wav" },
    { IDR_WAVE9, "assets/sound/missile_flight.wav" },
};

int cSoundWaveResource::Load (char const *szFilename)
{
    for ( int i = 0; i < _countof(resources); i++ ) {
        if ( strcmp(szFilename, resources[i].szResource) == 0 ) {
            szFilename = MAKEINTRESOURCE( resources[i].iResource );
            break;
        }
    }

    HMODULE hInstance = GetModuleHandle( NULL );
    HANDLE hInfo = FindResource( hInstance, szFilename, "WAVE" );
    DWORD dwSize = SizeofResource( hInstance, (HRSRC )hInfo );
    m_hResource = LoadResource( hInstance, (HRSRC )hInfo );
    m_lpvData = LockResource( m_hResource );

    // read wave data
    {
        riffChunk_t reader( (byte *)m_lpvData, dwSize );

        while ( reader.name( ) )
        {
            parseChunk( reader );
            reader.chunkNext( );
        }
    }

    UnlockResource( m_lpvData );
    FreeResource( m_hResource );
    
    return (m_numSamples > 0 ? ERROR_NONE : ERROR_FAIL);
}
