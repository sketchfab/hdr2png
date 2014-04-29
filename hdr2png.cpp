#include <sstream>
#include <unistd.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filter.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
OIIO_NAMESPACE_USING

/* Converts a float RGB pixel into byte RGB with a common exponent.
 * Source :http://www.graphics.cornell.edu/~bjw/rgbe.html
 */
void float2rgbe(uint8_t rgbe[4], float rgb[3])
{
    float v = rgb[0];
    if(rgb[1] > v)
        v = rgb[1];
    if(rgb[2] > v)
        v = rgb[2];

    if(v < 1e-32) {
        rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
    } else {
        int e;
        v = frexp(v, &e) * 256.0 / v;
        rgbe[0] = (unsigned char)(rgb[0] * v);
        rgbe[1] = (unsigned char)(rgb[1] * v);
        rgbe[2] = (unsigned char)(rgb[2] * v);
        rgbe[3] = (unsigned char)(e + 128);
    }
}

void hdr2png(ImageBuf& src, ImageBuf& dst) {
    ImageSpec spec = src.spec();
    int width = spec.width,
        height = spec.height;
    ImageSpec spec_out(width, height, 4, TypeDesc::UINT8);

    ImageBuf::Iterator<float, float> q(src, 0, width, 0, height);
    for (ImageBuf::Iterator<uint8_t, uint8_t> p(dst, 0, width, 0, height); p.valid(); p++, q++) { 
        q.pos(p.x(), p.y(), p.z());

        float* pixel_in = (float*)q.rawptr();
        uint8_t* pixel_out = (uint8_t*)p.rawptr();

        float2rgbe(pixel_out, pixel_in);
    }
}

void resize(ImageBuf& imageBuf, ImageBuf& dst) {
    static Filter2D *filter = NULL;
    for (int i = 0, n = Filter2D::num_filters(); i < n; i++) {
        FilterDesc fd;
        Filter2D::get_filterdesc(i, &fd);
        if (fd.name == "gaussian") {
            filter = Filter2D::create(fd.name, 2, 2);
            break;
        }
    }

    ImageSpec spec_out = imageBuf.spec();
    int width = dst.spec().width,
        height = dst.spec().height;

    ImageBufAlgo::resize(dst, imageBuf, dst.xbegin(), dst.xend(), dst.ybegin(), dst.yend(), filter);
}

int lowerPowerOfTwo(int v) {
    return (unsigned)pow(2, trunc(log2(v)));
}

void image_resize(char* filename_in, char* filename_out, int width, int height) {
    ImageBuf imageBuf_in(filename_in);
    imageBuf_in.read();
    ImageSpec spec = imageBuf_in.spec();

    ImageSpec spec_out = spec;
    spec_out.full_x = 0;
    spec_out.full_y = 0;
    spec_out.width = width;
    spec_out.height = height;
    spec_out.full_width = width;
    spec_out.full_height = height;
    spec_out.attribute("oiio:ColorSpace", "Linear");
    ImageBuf imageBuf_out(filename_out, spec_out);

    resize(imageBuf_in, imageBuf_out);

    ImageSpec spec_out2(width, height, 4, TypeDesc::UINT8);
    spec_out2.attribute("oiio:UnassociatedAlpha", true);
    ImageBuf imageBuf_out2(filename_out, spec_out2);
    hdr2png(imageBuf_out, imageBuf_out2);
    imageBuf_out2.save();
}

void makePowerOfTwo(char* filename_in, char* prefix_filename_out, int resolution_min) {
    ImageBuf imageBuf_in(filename_in);
    imageBuf_in.read();
    ImageSpec spec = imageBuf_in.spec();

    int width = lowerPowerOfTwo(spec.width),
        height = lowerPowerOfTwo(spec.height);
    do {
        std::string filename_out;
        std::ostringstream os;
        os << prefix_filename_out << "_" << width << "x" << height << ".png";
        std::cout << os.str();
        std::flush(std::cout);

        ImageSpec spec_out = spec;
        spec_out.full_x = 0;
        spec_out.full_y = 0;
        spec_out.width = width;
        spec_out.height = height;
        spec_out.full_width = width;
        spec_out.full_height = height;
        spec_out.attribute("oiio:ColorSpace", "Linear");
        ImageBuf imageBuf_out(filename_out, spec_out);
        resize(imageBuf_in, imageBuf_out);

        ImageSpec spec_out2(width, height, 4, TypeDesc::UINT8);
        spec_out2.attribute("oiio:UnassociatedAlpha", true);
        ImageBuf imageBuf_out2(os.str(), spec_out2);
        hdr2png(imageBuf_out, imageBuf_out2);
        imageBuf_out2.save();

        std::cout << std::endl;
        width /= 2;
        height /= 2;
    } while(width >= resolution_min && height >= resolution_min);
}

void print_infos(char* filename_in) {
    ImageBuf imageBuf_in(filename_in);
    imageBuf_in.read();
    ImageSpec spec = imageBuf_in.spec();

    std::cout << "Width=" << spec.width << std::endl;
    std::cout << "Height=" << spec.height << std::endl;

    std::cout << "Channels=" << spec.nchannels << std::endl;
    std::cout << "Format=";
    for(std::vector<std::string>::iterator it = spec.channelnames.begin(); it < spec.channelnames.end(); it++)
        std::cout << *it;
    std::cout << std::endl;

    std::cout << "Type=" << spec.format << std::endl;
}

void usage(int argc, char* argv[]) {
    std::cout << "Usage: " << argv[0] << " -i input.hdr -o out -m 128" << std::endl;
    std::cout << "Resizes input.hdr into lower power of two sizes, until Wx128 or 128xH" << std::endl;
    std::cout << "And output each resized images as out_WxH.png and RGBE png." << std::endl;
}

int main(int argc, char* argv[]) {
    char* filename_in = NULL;
    char* filename_out = NULL;
    int resolution_min = 0;
    int resize_width = 0,
        resize_height = 0;
    bool print_infos_flag = false;
    int c;

    while ((c = getopt(argc, argv, "p::i:o:m:w:h:")) != -1) {
        switch (c) {
        case 'p':
            print_infos_flag = true;
            break;
        case 'i':
            filename_in = optarg;
            break;
        case 'o':
            filename_out = optarg;
            break;
        case 'm':
            resolution_min = atoi(optarg);
            break;
        case 'w':
            resize_width = atoi(optarg);
            break;
        case 'h':
            resize_height = atoi(optarg);
            break;
        case '?':
        default:
            usage(argc, argv);
            return 1;
        }
    }

    if (print_infos_flag) {
        print_infos(filename_in);
        return 0;
    }

    if (filename_in == NULL || filename_out == NULL || (resolution_min == 0 && (resize_width == 0 || resize_height == 0))) {
        usage(argc, argv);
        return 1;
    }

    if (resolution_min > 0) {
        makePowerOfTwo(filename_in, filename_out, resolution_min);
    } else {
        image_resize(filename_in, filename_out, resize_width, resize_height);
    }

    return 0;
}
