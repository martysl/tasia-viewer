#pragma once

enum EType
{
    eSaveFile, eOpenFile, eOpenMultiple
};

enum ESaveFilter
{
    FFSAVE_ALL = 1,
    FFSAVE_WAV = 3,
    FFSAVE_TGA = 4,
    FFSAVE_BMP = 5,
    FFSAVE_AVI = 6,
    FFSAVE_ANIM = 7,
    FFSAVE_GLTF = 8,
    FFSAVE_XML = 9,
    FFSAVE_COLLADA = 10,
    FFSAVE_RAW = 11,
    FFSAVE_J2C = 12,
    FFSAVE_PNG = 13,
    FFSAVE_JPEG = 14,
    FFSAVE_SCRIPT = 15,
    FFSAVE_TGAPNG = 16,

    // Firestorm additions
    FFSAVE_BEAM = 50,
    FFSAVE_EXPORT = 51,
    FFSAVE_CSV = 52
};

enum ELoadFilter
{
    FFLOAD_ALL = 1,
    FFLOAD_WAV = 2,
    FFLOAD_IMAGE = 3,
    FFLOAD_ANIM = 4,
    FFLOAD_GLTF = 5,
    FFLOAD_XML = 6,
    FFLOAD_SLOBJECT = 7,
    FFLOAD_RAW = 8,
    FFLOAD_MODEL = 9,
    FFLOAD_COLLADA = 10,
    FFLOAD_SCRIPT = 11,
    FFLOAD_DICTIONARY = 12,
    FFLOAD_DIRECTORY = 13,   // To call from lldirpicker.
    FFLOAD_EXE = 14,          // Note: EXE will be treated as ALL on Windows and Linux but not on Darwin
    FFLOAD_MATERIAL = 15,
    FFLOAD_MATERIAL_TEXTURE = 16,
    FFLOAD_HDRI = 17,

    // Firestorm additions
    FFLOAD_IMPORT = 50
};