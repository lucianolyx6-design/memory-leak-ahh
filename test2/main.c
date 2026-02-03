/*
 * Frame Buffer Rainbow Gradient Program
 * 
 * This program directly writes to the frame buffer to create a smooth rainbow 
 * gradient across the entire screen. The rainbow transitions from red through 
 * orange, yellow, green, cyan, blue, and magenta.
 * 
 * Platform-specific compilation and execution:
 * 
 * LINUX:
 *   Compile: gcc -o rainbow main.c
 *   Run: sudo ./rainbow
 *   Note: This requires root privileges to access /dev/fb0
 * 
 * WINDOWS:
 *   Compile: gcc -o rainbow.exe main.c -luser32
 *   Run: rainbow.exe
 *   Note: Creates a fullscreen window and sets pixels directly
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-specific includes */
#ifdef __linux__
    /* Linux frame buffer headers */
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <linux/fb.h>
    #include <sys/mman.h>
#elif defined(_WIN32) || defined(_WIN64)
    /* Windows headers for graphics operations */
    #include <windows.h>
#else
    #error "Unsupported platform. This program requires Linux or Windows."
#endif

/* Forward declarations for platform-specific main functions */
#ifdef __linux__
int main_linux(void);
#endif

#ifdef _WIN32
int main_windows(void);
#endif

/* Structure to hold a single RGB pixel color */
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} RGB;

/*
 * Function: hsv_to_rgb
 * 
 * Converts HSV (Hue, Saturation, Value) color space to RGB color space.
 * HSV is useful for creating gradients because hue directly corresponds
 * to the color spectrum (0° = red, 120° = green, 240° = blue, etc).
 * 
 * Parameters:
 *   h: Hue in degrees (0.0 to 360.0)
 *   s: Saturation (0.0 to 1.0) - how intense the color is
 *   v: Value (0.0 to 1.0) - brightness of the color
 * 
 * Returns: RGB structure with red, green, and blue components (0-255)
 */
RGB hsv_to_rgb(float h, float s, float v)
{
    RGB color;
    float c = v * s;  /* Chroma: the color intensity component */
    float hh = h / 60.0f;  /* Scale hue to 0-6 range */
    float x = c * (1 - ((int)hh % 2 == 0 ? hh - (int)hh : 1 - (hh - (int)hh)));
    float m = v - c;  /* Match value: brings color to desired brightness */

    /* Determine which sextant of the color wheel we're in */
    if (hh < 1) {
        color.red = (unsigned char)((c + m) * 255);
        color.green = (unsigned char)((x + m) * 255);
        color.blue = (unsigned char)(m * 255);
    } else if (hh < 2) {
        color.red = (unsigned char)((x + m) * 255);
        color.green = (unsigned char)((c + m) * 255);
        color.blue = (unsigned char)(m * 255);
    } else if (hh < 3) {
        color.red = (unsigned char)(m * 255);
        color.green = (unsigned char)((c + m) * 255);
        color.blue = (unsigned char)((x + m) * 255);
    } else if (hh < 4) {
        color.red = (unsigned char)(m * 255);
        color.green = (unsigned char)((x + m) * 255);
        color.blue = (unsigned char)((c + m) * 255);
    } else if (hh < 5) {
        color.red = (unsigned char)((x + m) * 255);
        color.green = (unsigned char)(m * 255);
        color.blue = (unsigned char)((c + m) * 255);
    } else {
        color.red = (unsigned char)((c + m) * 255);
        color.green = (unsigned char)(m * 255);
        color.blue = (unsigned char)((x + m) * 255);
    }

    return color;
}

int main()
{
#ifdef __linux__
    return main_linux();
#elif defined(_WIN32) || defined(_WIN64)
    return main_windows();
#endif
}

/*
 * ============================================================================
 * LINUX IMPLEMENTATION
 * ============================================================================
 */
#ifdef __linux__

int main_linux()
{
    /* Declare the frame buffer file descriptor and variable info structures */
    int fb_fd;
    struct fb_var_screeninfo var_info;
    struct fb_fix_screeninfo fix_info;
    unsigned char *fb_data;  /* Pointer to the frame buffer memory */
    
    /* Open the frame buffer device for reading and writing */
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Failed to open /dev/fb0. Make sure you're running with sudo.");
        return 1;
    }

    /* 
     * ioctl(FBIOGET_VSCREENINFO) retrieves the variable screen information
     * This tells us:
     *  - xres: horizontal resolution in pixels
     *  - yres: vertical resolution in pixels
     *  - bits_per_pixel: color depth (usually 32 for 24-bit RGB + 8-bit alpha)
     */
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var_info) == -1) {
        perror("ioctl FBIOGET_VSCREENINFO");
        close(fb_fd);
        return 1;
    }

    /* 
     * ioctl(FBIOGET_FSCREENINFO) retrieves fixed screen information
     * This tells us:
     *  - smem_len: total size of the frame buffer memory in bytes
     *  - line_length: number of bytes per scanline (row)
     */
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix_info) == -1) {
        perror("ioctl FBIOGET_FSCREENINFO");
        close(fb_fd);
        return 1;
    }

    /* 
     * mmap() maps the frame buffer device memory into our process's address space
     * This allows us to directly write to video memory to update the screen
     * 
     * Parameters:
     *  - NULL: let the kernel choose the address
     *  - smem_len: size of memory to map (entire frame buffer)
     *  - PROT_READ | PROT_WRITE: we need read and write access
     *  - MAP_SHARED: changes are visible to other processes/hardware
     *  - fb_fd: file descriptor of /dev/fb0
     *  - 0: offset in the file (start from beginning)
     */
    fb_data = mmap(NULL, fix_info.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_data == MAP_FAILED) {
        perror("mmap failed");
        close(fb_fd);
        return 1;
    }

    /* Print detected screen information for debugging */
    printf("Frame Buffer Information:\n");
    printf("Resolution: %d x %d\n", var_info.xres, var_info.yres);
    printf("Bits per pixel: %d\n", var_info.bits_per_pixel);
    printf("Frame buffer size: %ld bytes\n", fix_info.smem_len);
    printf("Scanline length: %d bytes\n", fix_info.line_length);

    /* 
     * Main rendering loop: iterate through every pixel on the screen
     * 
     * Strategy: Use horizontal position (x) to determine the hue
     * This creates a smooth left-to-right rainbow gradient
     */
    for (unsigned int y = 0; y < var_info.yres; y++) {
        for (unsigned int x = 0; x < var_info.xres; x++) {
            /* 
             * Calculate the hue based on horizontal position
             * hue ranges from 0° (red) to 360° (magenta)
             * x ranges from 0 to xres-1, so divide by xres and multiply by 360
             */
            float hue = (x / (float)var_info.xres) * 360.0f;
            
            /* 
             * Convert HSV to RGB for the rainbow effect
             * Saturation = 1.0 (fully saturated, vivid colors)
             * Value = 1.0 (full brightness)
             */
            RGB pixel_color = hsv_to_rgb(hue, 1.0f, 1.0f);

            /* 
             * Calculate the memory offset for this pixel
             * 
             * Frame buffer memory is laid out sequentially:
             * - Row 0: pixels 0 to xres-1
             * - Row 1: pixels xres to 2*xres-1
             * - etc.
             * 
             * Each pixel takes (bits_per_pixel / 8) bytes
             * Offset = (y * line_length) + (x * bytes_per_pixel)
             */
            unsigned int bytes_per_pixel = var_info.bits_per_pixel / 8;
            unsigned long offset = (y * fix_info.line_length) + (x * bytes_per_pixel);

            /* 
             * Write the pixel color to frame buffer memory
             * Most systems use 32-bit pixels (ARGB or BGRA format)
             * We write: blue, green, red, alpha (in little-endian format)
             * 
             * The order might be BGR or RGB depending on the system,
             * but BGR (Blue-Green-Red) is most common on x86 systems
             */
            if (bytes_per_pixel == 4) {
                /* 32-bit color: ARGB or BGRA (try BGR first) */
                fb_data[offset + 0] = pixel_color.blue;      /* Blue channel */
                fb_data[offset + 1] = pixel_color.green;     /* Green channel */
                fb_data[offset + 2] = pixel_color.red;       /* Red channel */
                fb_data[offset + 3] = 255;                   /* Alpha (transparency) - fully opaque */
            } else if (bytes_per_pixel == 3) {
                /* 24-bit color: RGB */
                fb_data[offset + 0] = pixel_color.blue;
                fb_data[offset + 1] = pixel_color.green;
                fb_data[offset + 2] = pixel_color.red;
            }
        }
    }

    printf("Rainbow gradient written to frame buffer!\n");
    printf("Press Enter to exit and restore the display...\n");
    getchar();

    /* 
     * Clean up: unmap the frame buffer memory
     * This releases our access to the video memory
     */
    munmap(fb_data, fix_info.smem_len);

    /* Close the frame buffer device */
    close(fb_fd);

    return 0;
}

#endif /* __linux__ */

/*
 * ============================================================================
 * WINDOWS IMPLEMENTATION
 * ============================================================================
 */
#ifdef _WIN32

/* Global variables for Windows implementation */
HWND hwnd = NULL;
HDC hdc = NULL;
unsigned int screen_width = 0;
unsigned int screen_height = 0;

/*
 * Window procedure: handles window events and messages
 * This window cannot be closed by user interaction - it will persist indefinitely
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    /* Ignore all messages - the window will not close */
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main_windows()
{
    /* Get the screen dimensions */
    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);

    printf("Creating fullscreen window: %d x %d\n", screen_width, screen_height);

    /* Register the window class */
    const char CLASS_NAME[] = "Rainbow Window Class";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    /* Create a fullscreen window */
    hwnd = CreateWindowEx(
        WS_EX_TOPMOST,           /* Window is always on top */
        CLASS_NAME,
        "Rainbow Gradient",
        WS_POPUP,                /* Fullscreen, no decorations */
        0, 0,                    /* Position at 0,0 */
        screen_width,
        screen_height,
        NULL, NULL, NULL, NULL
    );

    if (hwnd == NULL) {
        printf("Failed to create window\n");
        return 1;
    }

    /* Display the window */
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    /* Get the device context for drawing */
    hdc = GetDC(hwnd);
    if (hdc == NULL) {
        printf("Failed to get device context\n");
        DestroyWindow(hwnd);
        return 1;
    }

    printf("Rendering rainbow gradient...\n");

    /* 
     * Main rendering loop: iterate through every pixel on the screen
     * 
     * Strategy: Use horizontal position (x) to determine the hue
     * This creates a smooth left-to-right rainbow gradient
     */
    for (unsigned int y = 0; y < screen_height; y++) {
        for (unsigned int x = 0; x < screen_width; x++) {
            /* 
             * Calculate the hue based on horizontal position
             * hue ranges from 0° (red) to 360° (magenta)
             */
            float hue = (x / (float)screen_width) * 360.0f;
            
            /* Convert HSV to RGB for the rainbow effect */
            RGB pixel_color = hsv_to_rgb(hue, 1.0f, 1.0f);

            /* 
             * Create a Windows COLORREF (0x00BGR format)
             * RGB() macro packs the color values into the correct format
             */
            COLORREF color = RGB(pixel_color.red, pixel_color.green, pixel_color.blue);

            /* Set the pixel at this location */
            SetPixel(hdc, x, y, color);
        }

        /* 
         * Process Windows messages to keep the window responsive
         * This allows the user to close the window or press a key to exit
         */
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    printf("Rainbow gradient displayed!\n");
    printf("Window is locked and cannot be closed. Use Ctrl+Alt+Delete or force-terminate the process to exit.\n");

    /* Infinite loop - the window will never close */
    while (1) {
        MSG msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

#endif /* _WIN32 */
