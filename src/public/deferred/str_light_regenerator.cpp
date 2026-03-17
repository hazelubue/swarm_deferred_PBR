

#include "str_light_regenerator.h"
#include "..\..\public\pixelwriter.h"

#ifdef CLIENT_DLL 
#include "cbase.h"
#endif 

void CDeferredLightDataRegenerator::RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect)
{
    CMatRenderContextPtr pRenderContext(materials);

    CPixelWriter writer;
    int width = pTexture->GetActualWidth();
    int height = pTexture->GetActualHeight();

    int x[2] = { 0, 1 };
    int y[2] = { 0, 0 };

    for (int i = 0; i < 2; i++) {
        writer.Seek(x[i], y[i]);

        writer.WritePixel(
            (uint8)(clamp(data[i * 4 + 0], 0, 1) * 255),
            (uint8)(clamp(data[i * 4 + 1], 0, 1) * 255),
            (uint8)(clamp(data[i * 4 + 2], 0, 1) * 255),
            (uint8)(clamp(data[i * 4 + 3], 0, 1) * 255)
        );
    }
}
