#include "camera_pipe.h"
#ifndef NO_AUTO_SCHEDULE
    #include "camera_pipe_auto_schedule.h"
#endif
#ifndef NO_GRADIENT_AUTO_SCHEDULE
    #include "camera_pipe_gradient_auto_schedule.h"
#endif

#include "benchmark_util.h"
#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "halide_malloc_trace.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

using namespace Halide::Runtime;
using namespace Halide::Tools;

int main(int argc, char **argv) {
    if (argc < 8) {
        printf("Usage: ./process raw.png color_temp gamma contrast sharpen timing_iterations output.png\n"
               "e.g. ./process raw.png 3700 2.0 50 1.0 5 output.png");
        return 0;
    }

#ifdef HL_MEMINFO
    halide_enable_malloc_trace();
#endif

    fprintf(stderr, "input: %s\n", argv[1]);
    Buffer<uint16_t> input = load_and_convert_image(argv[1]);
    fprintf(stderr, "       %d %d\n", input.width(), input.height());
    Buffer<uint8_t> output(((input.width() - 32) / 32) * 32, ((input.height() - 24) / 32) * 32, 3);

#ifdef HL_MEMINFO
    info(input, "input");
    stats(input, "input");
    // dump(input, "input");
#endif

    // These color matrices are for the sensor in the Nokia N900 and are
    // taken from the FCam source.
    float _matrix_3200[][4] = {{1.6697f, -0.2693f, -0.4004f, -42.4346f},
                               {-0.3576f, 1.0615f, 1.5949f, -37.1158f},
                               {-0.2175f, -1.8751f, 6.9640f, -26.6970f}};

    float _matrix_7000[][4] = {{2.2997f, -0.4478f, 0.1706f, -39.0923f},
                               {-0.3826f, 1.5906f, -0.2080f, -25.4311f},
                               {-0.0888f, -0.7344f, 2.2832f, -20.0826f}};
    Buffer<float> matrix_3200(4, 3), matrix_7000(4, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            matrix_3200(j, i) = _matrix_3200[i][j];
            matrix_7000(j, i) = _matrix_7000[i][j];
        }
    }

    //convert_and_save_image(matrix_3200, "../images/matrix_3200.mat");
    //convert_and_save_image(matrix_7000, "../images/matrix_7000.mat");

    Halide::Runtime::Buffer<float> matrix_3200_saved = load_and_convert_image("../images/matrix_3200.mat");
    bool success = true;
    float epsilon = 0.0001;
    matrix_3200.for_each_element([&](int x, int y) {
        float saved = matrix_3200_saved(x, y);
        float computed = matrix_3200(x, y);
        std::cerr << saved << " " << computed << "\n";
        if (std::abs(saved - computed) > epsilon) {
            printf("%d %d: %f vs %f\n", x, y, saved, computed);
            success = false;
        }
    });

    Halide::Runtime::Buffer<float> matrix_7000_saved = load_and_convert_image("../images/matrix_7000.mat");
    matrix_7000.for_each_element([&](int x, int y) {
        float saved = matrix_7000_saved(x, y);
        float computed = matrix_7000(x, y);
        if (std::abs(saved - computed) > epsilon) {
            printf("%d %d: %f vs %f\n", x, y, saved, computed);
            success = false;
        }
    });

    if (!success) {
        return -1;
    }

    float color_temp = (float) atof(argv[2]);
    float gamma = (float) atof(argv[3]);
    float contrast = (float) atof(argv[4]);
    float sharpen = (float) atof(argv[5]);
    int blackLevel = 25;
    int whiteLevel = 1023;

    multi_way_bench({
        {"camera_pipe Manual", [&]() { camera_pipe(input, matrix_3200, matrix_7000, color_temp, gamma, contrast, sharpen, blackLevel, whiteLevel, output); output.device_sync(); }},
#ifndef NO_AUTO_SCHEDULE
        {"camera_pipe Auto-scheduled", [&]() { camera_pipe_auto_schedule(input, matrix_3200, matrix_7000, color_temp, gamma, contrast, sharpen, blackLevel, whiteLevel, output); output.device_sync(); }},
#endif
#ifndef NO_GRADIENT_AUTO_SCHEDULE
        {"camera_pipe Gradient auto-scheduled", [&]() { camera_pipe_gradient_auto_schedule(input, matrix_3200, matrix_7000, color_temp, gamma, contrast, sharpen, blackLevel, whiteLevel, output); output.device_sync(); }}
#endif
        }
    );

    fprintf(stderr, "output: %s\n", argv[7]);
    convert_and_save_image(output, argv[7]);
    fprintf(stderr, "        %d %d\n", output.width(), output.height());

    printf("Success!\n");
    return 0;
}
