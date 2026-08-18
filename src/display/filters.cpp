#include "OEVector.h"

#include "ntsc.hpp"

typedef enum
{
    CANVAS_RGB,
    CANVAS_MONOCHROME,
    CANVAS_YUV,
    CANVAS_YIQ,
    CANVAS_CXA2025AS,
} CanvasDecoder;

#define NTSC_FSC        (315.0F / 88.0F * 1E6F)
#define NTSC_4FSC       (4 * NTSC_FSC)

#define NTSC_I_CUTOFF               1300000
#define NTSC_Q_CUTOFF               600000
#define NTSC_IQ_DELTA               (NTSC_I_CUTOFF - NTSC_Q_CUTOFF)

struct DisplayConfiguration {
    float videoBandwidth;
    float videoLumaBandwidth;
    float videoChromaBandwidth;
    float videoDecoder;
    float videoWhiteOnly;
    float imageSampleRate;
    float imageSubcarrier;
};

DisplayConfiguration displayConfiguration;

extern ntsc_config config ;

void init_display_configuration() {
    displayConfiguration.videoBandwidth = 4'500'000.0f;
    displayConfiguration.videoLumaBandwidth = 2'000'000.0f;
    displayConfiguration.videoChromaBandwidth = 600'000.0f;
    displayConfiguration.videoDecoder = CANVAS_YIQ;
    displayConfiguration.videoWhiteOnly = 0;
    displayConfiguration.imageSampleRate = NTSC_4FSC;
    displayConfiguration.imageSubcarrier = NTSC_FSC;
}

void generate_filters(int num_taps) {
    init_display_configuration();

    int total_samples = num_taps * 2 + 1;

    // Filters
    OEVector w = OEVector::chebyshevWindow(total_samples, CHEBYSHEV_SIDELOBE_DB); // orig 50.
    w = w.normalize();
    printf("Chebyshev window\n");
    w.print();
    
    OEVector wy, wu, wv;
    
    float bandwidth = displayConfiguration.videoBandwidth / displayConfiguration.imageSampleRate;
    

        float yBandwidth = displayConfiguration.videoLumaBandwidth / displayConfiguration.imageSampleRate;
        float uBandwidth = displayConfiguration.videoChromaBandwidth / displayConfiguration.imageSampleRate;
        float vBandwidth = uBandwidth;
        
printf("yBandwidth: %f\n", yBandwidth);
printf("uBandwidth: %f\n", uBandwidth);
printf("vBandwidth: %f\n", vBandwidth);

       /*  if (displayConfiguration.videoDecoder == CANVAS_YIQ) */
            uBandwidth = uBandwidth + NTSC_IQ_DELTA / displayConfiguration.imageSampleRate;
        
        // Switch to video bandwidth when no subcarrier
        if ((displayConfiguration.imageSubcarrier == 0.0) ||
            (displayConfiguration.videoWhiteOnly))
        {
            yBandwidth = bandwidth;
            uBandwidth = bandwidth;
            vBandwidth = bandwidth;
        }
        
        wy = w * OEVector::lanczosWindow(total_samples, yBandwidth);
        wy = wy.normalize();
        
        wu = w * OEVector::lanczosWindow(total_samples, uBandwidth);
        wu = wu.normalize() * 2;
        
        wv = w * OEVector::lanczosWindow(total_samples, vBandwidth);
        wv = wv.normalize() * 2;

    printf("wy\n");
    wy.print();
    printf("wu\n");
    wu.print();
    printf("wv\n");
    wv.print();

    for (int i = 0; i <= num_taps; i++) {
        int ind = i + num_taps;
        printf("wy[%d]: %f ", ind, wy.getValue(ind));
        config.filterCoefficients[i][0] = wy.getValue(ind);
        printf("wu[%d]: %f ", ind, wu.getValue(ind));
        config.filterCoefficients[i][1] = wu.getValue(ind);
        printf("wv[%d]: %f\n", ind, wv.getValue(ind));
        config.filterCoefficients[i][2] = wv.getValue(ind);
    }

}