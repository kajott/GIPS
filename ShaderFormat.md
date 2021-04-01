# GIPS Shader Format

GIPS filters are written as text files, using normal
[GLSL](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.3.30.pdf)
syntax for fragment shaders.
In fact, the code in the shader file will be passed verbatim
to the OpenGL driver's shader compiler;
GIPS just adds a bit of boilerplate to the beginning and the end.

## Entry Point

The main function of a GIPS filter shader is not `main`;
that is provided by GIPS itself as part of the boilerplate.
Instead, it's called `run`, has a single parameter,
and depending on the parameter and result type,
various usage scenarios are possible:

- `vec4 run(vec2 position) {...}`\
  The most generic version.
  The input parameter is the pixel position,
  and the output is the RGBA color.
- `vec3 run(vec2 position) {...}`\
  Same, but output only RGB;
  alpha will be set to 1.0 (full opacity).
- `vec4 run(vec4 color) {...}`\
  This is useful for filters that only perform color corrections.
  It receives the RGBA color of the pixel
  and returns the output RGBA color.
- `vec3 run(vec4 color) {...}`\
  As above, but output RGB only and keep the original alpha value.
- `vec4 run(vec3 color) {...}`\
  The opposite direction: Receives the RGB color (alpha is ignored),
  and outputs RGBA.
- `vec3 run(vec3 color) {...}`\
  Finally, this version has RGB on input _and_ output.
  The alpha value will be passed through unchanged.

The variants that receive a position need to read the input pixels themselves,
but they can read neighbor pixels or perform geometric distortions while doing so.
This is done using the function `vec4 pixel(vec2 position)`
that is defined in GIPS' boilerplate section.
It looks up the RGBA color in the source image at the specified position.

Pixels outside of the image area can be addressed;
they will return the color of the closest pixel on the edge
(i.e. "repetitive padding", or "clamp to edge").


## Parameter Definitions

Parameters are simply defined using GLSL `uniform` variables
of type `float`, `vec2`, `vec3` or `vec4`. No other types are supported.
Default values can be specified using the normal GLSL syntax.

By default, parameters appear in the UI as single to 4-element slider controls
under the name of the uniform variable, and with a range of 0.0 to 1.0.
This can be customized using a **parameter comment**,
which is the comment _immediately following_ the uniform variable definition.

The text of this comment is used for the parameter name,
but the following **tokens** are recognized, interpreted and removed before:

- `@min=<value>` and `@max=<value>`\
  Set the range for the sliders in the UI.
- `@digits=<number>`\
  Set the number of decimal digits of precision for the value's slider in the UI.
  By default, this is auto-detected from the value range,
  but in some cases, it might be desirable to set this manually.
- `@int`\
  Equivalent to `@digits=0`. Note that, despite the name,
  this doesn't turn the parameter into an `int` or `ivec` (it's still a `float`
  or floating-point `vec`), nor does it prevent the user from
  manually entering a value with more digits.
- `@color`\
  Turns the parameter UI of a `vec3` or `vec4` parameter
  into an RGB or RGBA color picker.
  Forbidden on other parameter types.
- `@angle`\
  Makes the parameter UI of a `float` parameter translate
  between degrees (that the user inputs)
  and radians (the actual value stored in the variable).\
  The `@min` and `@max` values are also specified in degrees;
  the default value, however, is still set in radians.
- `@switch` or `@toggle`\
  Turns the parameter UI of a `float` parameter into a checkbox
  that toggles the value of the parameter between 0.0 and 1.0.
  Forbidden on other parameter types.
- `@off=<value>` and `@on=<value>`\
  Can be used together with `@switch`/`@toggle` to specify different values
  to be used when the checkbox is unchecked or checked.
- `@unit=<name>`\
  Display a unit name after the value in the slider UI.
  `name` must be an alphanumeric string without any special characters
  or spaced in it. It is converted to lowercase.

### Parameter Examples

- `uniform float strength = 0.5;`\
  A simple value slider that goes from 0.0 to 1.0,
  with default at the middle.
- `uniform vec3 background = vec3(1.0, 0.0, 0.0);  // @color`\
  An RGB color that defaults to pure bright red.
- `uniform float sign = 1.0;  // invert @switch @off=1 @on=-1`\
  A checkbox that switches between values 1.0 if deactivated
  and -1.0 if activated. The GLSL variable is called `sign`,
  but the label of the checkbox in the UI will be "invert".


## Configuration Comments

GIPS not only peeks into comments after parameters;
it also scans all other comments for special tokens:

- `@version=1` or `@gips_version=1`\
  Specify the version of the GIPS shader format used for the filter.
  Currently, `1` is the only supported number.
- `@filter=<mode>`\
  Configure whether the `pixel()` function shall use interpolation or not:
  - If the value is `0`, `off`, `nearest` or `point`,
    no interpolation is performed. The color of the pixel closest
    to the specified coordinates will be returned.
  - If the value is `1`, `on`, `linear` or `bilinear`,
    bilinear filtering between pixels is performed. This is the default.
- `@coord=<mode>`\
  Specify the coordinate system used for the position input parameters
  of the `run` and `pixel` functions:
  - By default, i.e. if `@coord` is not set or set to `@coord=none`,
    simple normalized coordinates without aspect correction are used,
    with (0.0, 0.0) on the upper-left corner of the image
    and (1.0, 1.0) on the lower-right corner.
  - `@coord=pixel`\
    Configures a pixel-by-pixel coordinate system,
    with (0.0, 0.0) on the upper-left corner
    and (image width, image height) on the lower-right corner.\
    This is the ideal choice for filters that operate
    on a local pixel neighborhood.
  - `@coord=relative` or `@coord=rel`\
    Configures a "relative" coordinate system,
    with (0.0, 0.0) at the center of the image.
    The positive X and Y axes point towards the right and bottom, respectively.\
    The coordinates are normalized such that the value 1.0 or -1.0
    is reached on the short edge of the image;
    on the long edge, a higher value is reached,
    depending on the image's aspect ratio.\
    (This is similar to OpenGL's standard normalized device coordinate system,
    except that Y points downwards and aspect ratio correction is performed.)\
    This setting is useful for filters that perform larger-scale
    geometric distortions and don't need to address individual pixels.
- `@format=<format>`\
  Specify that the filter would like to use a color format with at least a
  certain amount of precision. The format affects the whole pipeline
  (not just the filter in question) and can be overridden by the user.
  Formats are strictly ordered, the higest requested format from all filters
  in the pipeline is chosen.\
  The supported formats are, ordered by priority from lowest to highest:
  - `@format=int8` or `@format=8`\
    8-bit integer per component (32 bits per pixel) - `GL_RGBA8`
  - `@format=int16` or `@format=16`\
    16-bit integer per component (64 bits per pixel) - `GL_RGBA16`
  - `@format=float16` or `@format=f16`\
    16-bit floating point per component (64 bits per pixel) - `GL_RGBA16F`
  - `@format=float32` or `@format=f32`\
    32-bit floating point per component (128 bits per pixel) - `GL_RGBA32F`

Note that the tokens for configuring the coordinate system and filtering
must be contained in comments **before** the `run` function.

Thus, it's generally a good idea to use a comment with all options
and a `@version` tag at the very beginning of the filter, like a header:

    // @gips_version=1 @coord=pixel @filter=off


## Predefined Functions and Uniform Variables

Other than the user-defined uniform variables, the GIPS boilerplate exposes
the following functions and uniform variables:

- `uniform vec2 gips_image_size`\
  The image size in pixels.
  Might be useful for custom coordinate computations in `@coord=none` mode,
  if a coordinate system other than those supported by `@coord` is desired.
- `uniform sampler2D gips_tex`\
  The input texture. Note that it's always top-down
  (i.e. (0,0) is on the upper-left corner).
- `vec4 pixel(in vec2 pos)`\
  Fetch a texel at the specified position in the coordinate system
  that has been set up in a `@coord` token before a pass's `run` function.\
  This is technically a wrapper around the GLSL `textureLod` function
  that operates on `gips_tex` with appropriate coordinate transformations
  and a LOD of 0.


## Multi-Pass Filters

Filters can contain multiple (up to four) shader passes
that are executed in sequence. The main functions are then not called `run`,
but `run_pass1`, `run_pass2` etc.

The passes can can have different signatures
and use different filtering and coordinate systems.
The settings for these must be specified in a comment preceding
the `run_passX` function for which they shall be set.
