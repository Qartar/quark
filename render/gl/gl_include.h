// gl_include.h
//

#pragma once

#include "gl/gl_types.h"

#include "win_include.h"
#include <GL/gl.h>

//------------------------------------------------------------------------------
// OpenGL 1.2

#define GL_BGR                          0x80E0

//------------------------------------------------------------------------------
// OpenGL 1.3

#define GL_TEXTURE0                     0x84C0

//------------------------------------------------------------------------------
// OpenGL 1.5

#define GL_ARRAY_BUFFER                 0x8892
#define GL_ELEMENT_ARRAY_BUFFER         0x8893

#define GL_READ_ONLY                    0x88B8
#define GL_WRITE_ONLY                   0x88B9
#define GL_READ_WRITE                   0x88BA

#define GL_STREAM_DRAW                  0x88E0
#define GL_STREAM_READ                  0x88E1
#define GL_STREAM_COPY                  0x88E2
#define GL_STATIC_DRAW                  0x88E4
#define GL_STATIC_READ                  0x88E5
#define GL_STATIC_COPY                  0x88E6
#define GL_DYNAMIC_DRAW                 0x88E8
#define GL_DYNAMIC_READ                 0x88E9
#define GL_DYNAMIC_COPY                 0x88EA

//------------------------------------------------------------------------------
// OpenGL 2.0

#define GL_CONSTANT_COLOR               0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR     0x8002
#define GL_CONSTANT_ALPHA               0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA     0x8004

#define GL_FRAGMENT_SHADER              0x8B30
#define GL_VERTEX_SHADER                0x8B31

#define GL_DELETE_STATUS                0x8B80
#define GL_COMPILE_STATUS               0x8B81
#define GL_LINK_STATUS                  0x8B82
#define GL_VALIDATE_STATUS              0x8B83
#define GL_INFO_LOG_LENGTH              0x8B84

//------------------------------------------------------------------------------
// OpenGL 3.0

#define GL_RGBA32F                      0x8814

#define GL_RGBA16F                      0x881A

#define GL_DEPTH24_STENCIL8             0x88F0

//------------------------------------------------------------------------------
// OpenGL 3.1

#define GL_UNIFORM_BUFFER               0x8A11

//------------------------------------------------------------------------------
// ARB_framebuffer_object

// Accepted by the <target> parameter of BindFramebuffer,
// CheckFramebufferStatus, FramebufferTexture{1D|2D|3D},
// FramebufferRenderbuffer, and
// GetFramebufferAttachmentParameteriv:

#define GL_FRAMEBUFFER                  0x8D40
#define GL_READ_FRAMEBUFFER             0x8CA8
#define GL_DRAW_FRAMEBUFFER             0x8CA9

// Accepted by the <target> parameter of BindRenderbuffer,
// RenderbufferStorage, and GetRenderbufferParameteriv, and
// returned by GetFramebufferAttachmentParameteriv:

#define GL_RENDERBUFFER                 0x8D41

// Accepted by the <attachment> parameter of
// FramebufferTexture{1D|2D|3D}, FramebufferRenderbuffer, and
// GetFramebufferAttachmentParameteriv

#define GL_COLOR_ATTACHMENT0            0x8CE0
#define GL_DEPTH_ATTACHMENT             0x8D00
#define GL_STENCIL_ATTACHMENT           0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT     0x821A

// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv,
// GetFloatv, and GetDoublev:

#define GL_MAX_SAMPLES                  0x8D57
#define GL_FRAMEBUFFER_BINDING          0x8CA6 // alias DRAW_FRAMEBUFFER_BINDING
#define GL_DRAW_FRAMEBUFFER_BINDING     0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING     0x8CAA
#define GL_RENDERBUFFER_BINDING         0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS        0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE        0x84E8

//------------------------------------------------------------------------------
// ARB_texture_multisample

// Accepted by the <target> parameter of BindTexture and
// TexImage2DMultisample:

#define GL_TEXTURE_2D_MULTISAMPLE       0x9100
