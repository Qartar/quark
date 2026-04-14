// gl_include.h
//

#pragma once

#include "gl/gl_types.h"

#include "win_include.h"
#include <GL/gl.h>

//------------------------------------------------------------------------------
// OpenGL 1.2

#define GL_UNSIGNED_INT_10_10_10_2      0x8036
#define GL_UNSIGNED_INT_2_10_10_10_REV  0x8368
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

#define GL_DEPTH24_STENCIL8             0x88F0
#define GL_MAP_READ_BIT                 0x0001
#define GL_MAP_WRITE_BIT                0x0002

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
// ARB_debug_output

// Tokens accepted by the <target> parameters of Enable, Disable,
// and IsEnabled:

#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB                     0x8242

// Tokens accepted by the <value> parameters of GetBooleanv,
// GetIntegerv, GetFloatv, and GetDoublev:

#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB                     0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB                    0x9144
#define GL_DEBUG_LOGGED_MESSAGES_ARB                        0x9145
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB             0x8243

// Tokens accepted by the <pname> parameter of GetPointerv:

#define GL_DEBUG_CALLBACK_FUNCTION_ARB                      0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB                    0x8245

// Tokens accepted or provided by the <source> parameters of
// DebugMessageControlARB, DebugMessageInsertARB and DEBUGPROCARB,
// and the <sources> parameter of GetDebugMessageLogARB:

#define GL_DEBUG_SOURCE_API_ARB                             0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB                   0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB                 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB                     0x8249
#define GL_DEBUG_SOURCE_APPLICATION_ARB                     0x824A
#define GL_DEBUG_SOURCE_OTHER_ARB                           0x824B

// Tokens accepted or provided by the <type> parameters of
// DebugMessageControlARB, DebugMessageInsertARB and DEBUGPROCARB,
// and the <types> parameter of GetDebugMessageLogARB:

#define GL_DEBUG_TYPE_ERROR_ARB                             0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB               0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB                0x824E
#define GL_DEBUG_TYPE_PORTABILITY_ARB                       0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_ARB                       0x8250
#define GL_DEBUG_TYPE_OTHER_ARB                             0x8251

// Tokens accepted or provided by the <severity> parameters of
// DebugMessageControlARB, DebugMessageInsertARB and DEBUGPROCARB
// callback functions, and the <severities> parameter of
// GetDebugMessageLogARB:

#define GL_DEBUG_SEVERITY_HIGH_ARB                          0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB                        0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB                           0x9148

//------------------------------------------------------------------------------
// ARB_texture_multisample

// Accepted by the <target> parameter of BindTexture and
// TexImage2DMultisample:

#define GL_TEXTURE_2D_MULTISAMPLE       0x9100

//------------------------------------------------------------------------------
// ARB_shader_storage_buffer_object

// Accepted by the <target> parameters of BindBuffer, BufferData,
// BufferSubData, MapBuffer, UnmapBuffer, GetBufferSubData, and
// GetBufferPointerv:

#define GL_SHADER_STORAGE_BUFFER                        0x90D2

//------------------------------------------------------------------------------
// WGL_ARB_create_context

// Accepted as an attribute name in <*attribList>:

#define WGL_CONTEXT_MAJOR_VERSION_ARB               0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB               0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB                 0x2093
#define WGL_CONTEXT_FLAGS_ARB                       0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB                0x9126

// Accepted as bits in the attribute value for WGL_CONTEXT_FLAGS in
// <*attribList>:

#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x0002

// Accepted as bits in the attribute value for
// WGL_CONTEXT_PROFILE_MASK_ARB in <*attribList>:

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   0x00000002

// New errors returned by GetLastError:

#define GL_ERROR_INVALID_VERSION_ARB                0x2095
#define GL_ERROR_INVALID_PROFILE_ARB                0x2096
