#pragma once

#include <cstdint>
#include "hid/usage.hpp"

namespace OB::HID
{
    namespace Page
    {
        enum class Keyboard
        {
            Reserved                   = 0x00,
            Error_RollOver             = 0x01,
            Post_Fail                  = 0x02,
            Error_Undefined            = 0x03,
            Keyboard_aA                = 0x04,
            Keyboard_bB                = 0x05,
            Keyboard_cC                = 0x06,
            Keyboard_dD                = 0x07,
            Keyboard_eE                = 0x08,
            Keyboard_fF                = 0x09,
            Keyboard_gG                = 0x0A,
            Keyboard_hH                = 0x0B,
            Keyboard_iI                = 0x0C,
            Keyboard_jJ                = 0x0D,
            Keyboard_kK                = 0x0E,
            Keyboard_lL                = 0x0F,
            Keyboard_mM                = 0x10,
            Keyboard_nN                = 0x11,
            Keyboard_oO                = 0x12,
            Keyboard_pP                = 0x13,
            Keyboard_qQ                = 0x14,
            Keyboard_rR                = 0x15,
            Keyboard_sS                = 0x16,
            Keyboard_tT                = 0x17,
            Keyboard_uU                = 0x18,
            Keyboard_vV                = 0x19,
            Keyboard_wW                = 0x1A,
            Keyboard_xX                = 0x1B,
            Keyboard_yY                = 0x1C,
            Keyboard_zZ                = 0x1D,
            Keyboard_1_excl            = 0x1E,
            Keyboard_2_at              = 0x1F,
            Keyboard_3_hash            = 0x20,
            Keyboard_4_dollar          = 0x21,
            Keyboard_5_percent         = 0x22,
            Keyboard_6_carte           = 0x23,
            Keyboard_7_ampersand       = 0x24,
            Keyboard_8_star            = 0x25,
            Keyboard_9_lParen          = 0x26,
            Keyboard_0_rParen          = 0x27,
            Keyboard_Return            = 0x28,
            Keyboard_Escape            = 0x29,
            Keyboard_Delete            = 0x2A,
            Keyboard_Tab               = 0x2B,
            Keyboard_Space             = 0x2C,
            Keyboard_minus_underscore  = 0x2D,
            Keyboard_equals_plus       = 0x2E,
            Keyboard_lBracket_lBrace   = 0x2F,
            Keyboard_rBracket_rBrace   = 0x30,
            Keyboard_bSlash_pipe       = 0x31,
            Keyboard_hash_grave        = 0x32,
            Keyboard_semicolon_colon   = 0x33,
            Keyboard_sglQuote_dblQuote = 0x34,
            Keyboard_graveAccent_tilde = 0x35,
            Keyboard_comma_lChevron    = 0x36,
            Keyboard_period_rChevron   = 0x37,
            Keyboard_fSlash_qMark      = 0x38,
            Keyboard_capsLock          = 0x39,
            Keyboard_F1                = 0x3A,
            Keyboard_F2                = 0x3B,
            Keyboard_F3                = 0x3C,
            Keyboard_F4                = 0x3D,
            Keyboard_F5                = 0x3E,
            Keyboard_F6                = 0x3F,
            Keyboard_F7                = 0x40,
            Keyboard_F8                = 0x41,
            Keyboard_F9                = 0x42,
            Keyboard_F10               = 0x43,
            Keyboard_F11               = 0x44,
            Keyboard_F12               = 0x45,

            Keyboard_LeftControl       = 0xE0,
            Keyboard_LeftShift         = 0xE1,
            Keyboard_LeftAlt           = 0xE2,
            Keyboard_LeftGUI           = 0xE3,
            Keyboard_RightControl      = 0xE4,
            Keyboard_RightShift        = 0xE5,
            Keyboard_RightAlt          = 0xE6,
            Keyboard_RightGUI          = 0xE7,
            
        };
    }

    template<>
    constexpr UsagePageInfo get_info<Page::Keyboard>() 
    {
        return UsagePageInfo{0x07};
    }

}