/*=========================================================
Name    :   snd_wav_source.h
Date    :   04/07/2006
=========================================================*/

#pragma once

#include "snd_files.h"
#include "cm_filesystem.h"

/*=========================================================
=========================================================*/

#define STREAM_THRESHOLD    0x10000     // 65k

constexpr int make_id(char a, char b, char c, char d)
{
    return (int(d) << 24) | (int(c) << 16) | (int(b) << 8) | (int(a));
}

constexpr int CHUNK_FMT     = make_id('f','m','t',' ');
constexpr int CHUNK_CUE     = make_id('c','u','e',' ');
constexpr int CHUNK_DATA    = make_id('d','a','t','a');

typedef class riffChunk_c
{
public:
    riffChunk_c (char const *szFilename);
    riffChunk_c (byte* pChunkData, int nChunkSize);
    riffChunk_c (riffChunk_c &Outer);

    bool        chunkNext ();

    unsigned int    name ();
    int             getSize ();

    int             getPos ();
    int             setPos (int pos);

    int     readChunk (byte *pOutput);
    int     readData (byte *pOutput, int nLength);
    int     readInt ();

private:
    void    chunkSet ();

    int     m_start;
    int     m_size;
    int     m_name;
    int     m_pos;

    int     m_read (void *out, int len);

    unsigned int    m_chunkName;
    int             m_chunkSize;
    int             m_chunkStart;

    file::stream m_riff;
    byte    *m_riffData;
} riffChunk_t;

class cSoundWaveSource : public cSoundSource
{
public:
    virtual int             getSamples (byte *pOutput, int nSamples, int nOffset, bool bLooping) = 0;
    virtual soundFormat_t   *getFormat () { return &m_format; }
    virtual char            *getFilename () { return m_szFilename; }
    virtual float           getLoopPosition (float flPosition);

    virtual result  Load (char const *szFilename) = 0;
    virtual void    Unload () = 0;

protected:
    void            parseChunk  (riffChunk_t &chunk);

    virtual void    parseFormat (riffChunk_t &chunk);
    virtual void    parseCue    (riffChunk_t &chunk);
    virtual void    parseData   (riffChunk_t &chunk) = 0;

    soundFormat_t   m_format;
    char            m_szFilename[LONG_STRING];

    int         m_numSamples;
    int         m_loopStart;
};
