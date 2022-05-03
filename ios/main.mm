/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "entry_p.h"


#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreVideo/CoreVideo.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/CAEAGLLayer.h>
#import <ARKit/ARKit.h>

#if __IPHONE_8_0 && !TARGET_IPHONE_SIMULATOR  // check if sdk/target supports metal
#   import <Metal/Metal.h>
#   import <QuartzCore/CAMetalLayer.h>
#   define HAS_METAL_SDK
#endif

#include <bgfx/platform.h>

#include <bx/uint32_t.h>
#include <bx/thread.h>
#include "bgfx_utils.h"
#include "Application.h"
#include <vector>

namespace entry
{
    struct MainThreadEntry
    {
        int m_argc;
        const char* const* m_argv;
        
        static int32_t threadFunc(bx::Thread* _thread, void* _userData);
    };

    static WindowHandle s_defaultWindow = { 0 };

    struct Context
    {
        Context(uint32_t _width, uint32_t _height)
        {
            static const char* const argv[] = { "ios" };
            m_mte.m_argc = BX_COUNTOF(argv);
            m_mte.m_argv = argv;

            m_eventQueue.postSizeEvent(s_defaultWindow, _width, _height);

            // Prevent render thread creation.
            bgfx::renderFrame();

            m_thread.init(MainThreadEntry::threadFunc, &m_mte);
        }

        ~Context()
        {
            m_thread.shutdown();
        }

        MainThreadEntry m_mte;
        bx::Thread m_thread;
        sam::Application *m_pApplication;

        EventQueue m_eventQueue;
    };

    static Context* s_ctx;

    int32_t MainThreadEntry::threadFunc(bx::Thread* _thread, void* _userData)
    {
        BX_UNUSED(_thread);

        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (mainBundle != nil)
        {
            CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
            if (resourcesURL != nil)
            {
                char path[PATH_MAX];
                if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX) )
                {
                    chdir(path);
                }

                CFRelease(resourcesURL);
            }
        }

        NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        NSString *fullPath = [documentsDirectory stringByAppendingPathComponent:@"UserDetails"];
        
        NSURL *pUrl = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
        NSString *docPath = pUrl.absoluteString;
        
        MainThreadEntry* self = (MainThreadEntry*)_userData;
        s_ctx->m_pApplication = main(self->m_argc, self->m_argv, [documentsDirectory UTF8String]);
        
        return 0;
    }

    const Event* poll()
    {
        return s_ctx->m_eventQueue.poll();
    }

    const Event* poll(WindowHandle _handle)
    {
        return s_ctx->m_eventQueue.poll(_handle);
    }

    void release(const Event* _event)
    {
        s_ctx->m_eventQueue.release(_event);
    }

    WindowHandle createWindow(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height, uint32_t _flags, const char* _title)
    {
        BX_UNUSED(_x, _y, _width, _height, _flags, _title);
        WindowHandle handle = { UINT16_MAX };
        return handle;
    }

    void destroyWindow(WindowHandle _handle)
    {
        BX_UNUSED(_handle);
    }

    void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
    {
        BX_UNUSED(_handle, _x, _y);
    }

    void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height)
    {
        BX_UNUSED(_handle, _width, _height);
    }

    void setWindowTitle(WindowHandle _handle, const char* _title)
    {
        BX_UNUSED(_handle, _title);
    }

    void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
    {
        BX_UNUSED(_handle, _flags, _enabled);
    }

    void toggleFullscreen(WindowHandle _handle)
    {
        BX_UNUSED(_handle);
    }

    void setMouseLock(WindowHandle _handle, bool _lock)
    {
        BX_UNUSED(_handle, _lock);
    }

} // namespace entry

using namespace entry;

#ifdef HAS_METAL_SDK
static    id<MTLDevice>  m_device = NULL;
#else
static    void* m_device = NULL;
#endif

@interface ViewController : UIViewController
@end
@implementation ViewController
- (BOOL)prefersStatusBarHidden
{
    return YES;
}
@end


@interface View : UIView
{
    CADisplayLink* m_displayLink;
}

@end

@implementation View

+ (Class)layerClass
{
#ifdef HAS_METAL_SDK
    Class metalClass = NSClassFromString(@"CAMetalLayer");    //is metal runtime sdk available
    if ( metalClass != nil)
    {
        m_device = MTLCreateSystemDefaultDevice(); // is metal supported on this device (is there a better way to do this - without creating device ?)
        if (m_device)
        {
            [m_device retain];
            return metalClass;
        }
    }
#endif

    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)rect
{
    self = [super initWithFrame:rect];

    if (nil == self)
    {
        return nil;
    }

    bgfx::PlatformData pd;
    pd.ndt          = NULL;
    pd.nwh          = self.layer;
    pd.context      = m_device;
    pd.backBuffer   = NULL;
    pd.backBufferDS = NULL;
    bgfx::setPlatformData(pd);

    return self;
}

- (void)layoutSubviews
{
    uint32_t frameW = (uint32_t)(self.contentScaleFactor * self.frame.size.width);
    uint32_t frameH = (uint32_t)(self.contentScaleFactor * self.frame.size.height);
    s_ctx->m_eventQueue.postSizeEvent(s_defaultWindow, frameW, frameH);
}

- (void)start
{
    if (nil == m_displayLink)
    {
        m_displayLink = [self.window.screen displayLinkWithTarget:self selector:@selector(renderFrame)];
        //[m_displayLink setFrameInterval:1];
        //[m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        //        [m_displayLink addToRunLoop:[NSRunLoop currentRunLoop]];
        [m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    }
}

- (void)stop
{
    if (nil != m_displayLink)
    {
        [m_displayLink invalidate];
        m_displayLink = nil;
    }
}

- (void)renderFrame
{
    bgfx::renderFrame();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    BX_UNUSED(touches);
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    touchLocation.x *= self.contentScaleFactor;
    touchLocation.y *= self.contentScaleFactor;

    s_ctx->m_eventQueue.postMouseEvent(s_defaultWindow, touchLocation.x, touchLocation.y, 0);
    s_ctx->m_eventQueue.postMouseEvent(s_defaultWindow, touchLocation.x, touchLocation.y, 0, MouseButton::Left, true);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    BX_UNUSED(touches);
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    touchLocation.x *= self.contentScaleFactor;
    touchLocation.y *= self.contentScaleFactor;

    s_ctx->m_eventQueue.postMouseEvent(s_defaultWindow, touchLocation.x, touchLocation.y, 0, MouseButton::Left, false);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    BX_UNUSED(touches);
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    touchLocation.x *= self.contentScaleFactor;
    touchLocation.y *= self.contentScaleFactor;

    s_ctx->m_eventQueue.postMouseEvent(s_defaultWindow, touchLocation.x, touchLocation.y, 0);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    BX_UNUSED(touches);
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint touchLocation = [touch locationInView:self];
    touchLocation.x *= self.contentScaleFactor;
    touchLocation.y *= self.contentScaleFactor;

    s_ctx->m_eventQueue.postMouseEvent(s_defaultWindow, touchLocation.x, touchLocation.y, 0, MouseButton::Left, false);
}

@end

@interface ARDelegate : NSObject<ARSessionDelegate>
{
    
}
@end

@interface AppDelegate : UIResponder<UIApplicationDelegate>
{
    UIWindow* m_window;
    View* m_view;
    dispatch_queue_t m_sessionQueue;
    dispatch_queue_t m_dataQueue;
    ARSession *m_arSession;
    ARDelegate *m_arDelegate;
}

@property (nonatomic, retain) UIWindow* m_window;
@property (nonatomic, retain) View* m_view;

@end

@implementation AppDelegate

@synthesize m_window;
@synthesize m_view;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    BX_UNUSED(application, launchOptions);

    CGRect rect = [ [UIScreen mainScreen] bounds];
    m_window = [ [UIWindow alloc] initWithFrame: rect];
    m_view = [ [View alloc] initWithFrame: rect];

    [m_window addSubview: m_view];

    UIViewController *viewController = [[ViewController alloc] init];
    viewController.view = m_view;

    [m_window setRootViewController:viewController];
    [m_window makeKeyAndVisible];

    [m_window makeKeyAndVisible];

    float scaleFactor = [[UIScreen mainScreen] scale];
    [m_view setContentScaleFactor: scaleFactor ];

    s_ctx = new Context((uint32_t)(scaleFactor*rect.size.width), (uint32_t)(scaleFactor*rect.size.height));

    m_sessionQueue =
        dispatch_queue_create("sessionqueue", nullptr);
    dispatch_async(m_sessionQueue,  ^(void){
        [self configureARSession];
    });
    
    
    
    return YES;
}

- (void)configureARSession
{
    m_arSession = [[ARSession alloc] init];
    m_arDelegate = [[ARDelegate alloc] init];
    
    ARFaceTrackingConfiguration *config = [[ARFaceTrackingConfiguration alloc] init];
    config.lightEstimationEnabled = true;
    NSArray<ARVideoFormat *> *pVidFormts = ARFaceTrackingConfiguration.supportedVideoFormats;
    unsigned long cnt = [pVidFormts count];
    unsigned long foundIdx = -1;
    float minwidth = 100000;
    for (unsigned long fidx = 0; fidx < cnt; fidx++)
    {
        CGSize res = pVidFormts[fidx].imageResolution;
        if (res.width < minwidth)
        {
            minwidth = res.width;
            foundIdx = fidx;
        }
    }
    config.videoFormat = pVidFormts[0];

    m_arSession.delegate = m_arDelegate;
    [m_arSession runWithConfiguration:config];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    BX_UNUSED(application);
    s_ctx->m_eventQueue.postSuspendEvent(s_defaultWindow, Suspend::WillSuspend);
    [m_view stop];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    BX_UNUSED(application);
    s_ctx->m_eventQueue.postSuspendEvent(s_defaultWindow, Suspend::DidSuspend);
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    BX_UNUSED(application);
    s_ctx->m_eventQueue.postSuspendEvent(s_defaultWindow, Suspend::WillResume);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    BX_UNUSED(application);
    s_ctx->m_eventQueue.postSuspendEvent(s_defaultWindow, Suspend::DidResume);
    [m_view start];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    BX_UNUSED(application);
    [m_view stop];
}

- (void)dealloc
{
    [m_window release];
    [m_view release];
    [super dealloc];
}

@end

int main(int _argc, char * _argv[])
{
    NSAutoreleasePool* pool = [ [NSAutoreleasePool alloc] init];
    int exitCode = UIApplicationMain(_argc, (char**)_argv, @"UIApplication", NSStringFromClass([AppDelegate class]) );
    [pool release];
    return exitCode;
}

struct YCrCbData
{
    int yOffset;
    int yRowBytes;
    int cbCrOffset;
    int cbCrRowBytes;
};

@implementation ARDelegate
- (void)session:(ARSession *)session
 didUpdateFrame:(ARFrame *)frame
{
    AVDepthData *pDepthData = frame.capturedDepthData;
    if (pDepthData == nil)
        return;
    
    AVCameraCalibrationData *cameraCalibrationData =
        [pDepthData cameraCalibrationData];
    matrix_float3x3 cameraMatrix = [cameraCalibrationData intrinsicMatrix];
    float mtxarray[11];
    for (int i = 0; i < 9; ++i)
    {
        mtxarray[i] = cameraMatrix.columns[i/3][i%3];
    }
    
    CGSize size = [cameraCalibrationData intrinsicMatrixReferenceDimensions];
    mtxarray[9] = size.width;
    mtxarray[10] = size.height;
    
    CVPixelBufferRef imgBuffer = [frame capturedImage];

    CVPixelBufferRef depthBuffer = pDepthData.depthDataMap;
    if (CVPixelBufferGetPixelFormatType(imgBuffer) == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange &&
        CVPixelBufferGetPixelFormatType(depthBuffer) == kCVPixelFormatType_DepthFloat32)
    {
        CVPixelBufferLockBaseAddress(imgBuffer, kCVPixelBufferLock_ReadOnly);
        size_t datasize = CVPixelBufferGetDataSize(imgBuffer);
        void *pVidBuffer = CVPixelBufferGetBaseAddress(imgBuffer);
        CVPlanarPixelBufferInfo_YCbCrBiPlanar *bufferInfo = (CVPlanarPixelBufferInfo_YCbCrBiPlanar *)pVidBuffer;
        YCrCbData cbd;
        // Get the offsets and bytes per row.
        cbd.yOffset =  CFSwapInt32BigToHost(bufferInfo->componentInfoY.offset);
        cbd.yRowBytes = CFSwapInt32BigToHost(bufferInfo->componentInfoY.rowBytes);
        cbd.cbCrOffset = CFSwapInt32BigToHost(bufferInfo->componentInfoCbCr.offset);
        cbd.cbCrRowBytes = CFSwapInt32BigToHost(bufferInfo->componentInfoCbCr.rowBytes);
        
        sam::DepthData ddata;
        ddata.vidData.resize(datasize - cbd.yOffset + sizeof(YCrCbData));
        memcpy(ddata.vidData.data(), &cbd, + sizeof(YCrCbData));
        memcpy(ddata.vidData.data() + + sizeof(YCrCbData), ((char *)pVidBuffer) + cbd.yOffset, ddata.vidData.size());
        CVPixelBufferUnlockBaseAddress(imgBuffer, kCVPixelBufferLock_ReadOnly);
    
        
        datasize = CVPixelBufferGetDataSize(depthBuffer);
        ddata.depthData.resize(datasize / sizeof(float) +  16);
        memcpy(ddata.depthData.data(), mtxarray, sizeof(mtxarray));
        
        CVPixelBufferLockBaseAddress(depthBuffer, kCVPixelBufferLock_ReadOnly);
        void *pDepthBuffer = CVPixelBufferGetBaseAddress(depthBuffer);
        memcpy(ddata.depthData.data() + 16, pDepthBuffer, ddata.depthData.size() * sizeof(float));
        CVPixelBufferUnlockBaseAddress(depthBuffer, kCVPixelBufferLock_ReadOnly);
        
        ddata.props.timestamp = frame.timestamp;
        ddata.props.depthWidth = CVPixelBufferGetWidth(depthBuffer);
        ddata.props.depthHeight = CVPixelBufferGetHeight(depthBuffer);
        ddata.props.vidWidth = CVPixelBufferGetWidth(imgBuffer);
        ddata.props.vidHeight = CVPixelBufferGetHeight(imgBuffer);
        ddata.props.depthMode = ddata.props.vidMode = 0;
        entry::s_ctx->m_pApplication->OnDepthBuffer(ddata);
    }
}

void GetMat(const simd_float4x4 &m, float f[16] )
{
    for (int i = 0; i < 16; ++i)
    {
        f[i] = m.columns[i/4][i%4];
    }
}
- (void)session:(ARSession *)session
didUpdateAnchors:(NSArray<__kindof ARAnchor *> *)anchors
{
    for (int idx = 0; idx < anchors.count; ++idx)
    {
        if ([anchors[idx] isKindOfClass:[ARFaceAnchor class]])
        {
            sam::FaceDataProps fdp;
            fdp.timestamp = session.currentFrame.timestamp;
            ARFaceAnchor *faceAnchor = (ARFaceAnchor *)anchors[idx];
            ARFaceGeometry *faceGmt = (ARFaceGeometry *)faceAnchor.geometry;
            ARCamera *camera = session.currentFrame.camera;
            simd_float4x4 viewMat = [camera viewMatrixForOrientation:(UIInterfaceOrientationPortrait)];
            simd_float4x4 projMat = [camera projectionMatrixForOrientation:(UIInterfaceOrientationPortrait) viewportSize:(CGSize {480, 640 }) zNear:(0.1f) zFar:(10.0f)];
            GetMat(viewMat, fdp.viewMatf);
            GetMat(faceAnchor.transform, fdp.wMatf);
            GetMat(projMat, fdp.projMatf);
            std::vector<float> vertices;
            const simd_float3 *pvertices = faceGmt.vertices;
            const simd_float2 *ptex = faceGmt.textureCoordinates;
            int count = faceGmt.vertexCount;
            for (int idx = 0; idx < count; ++idx)
            {
                vertices.push_back(pvertices[idx][0]);
                vertices.push_back(pvertices[idx][1]);
                vertices.push_back(pvertices[idx][2]);
                vertices.push_back(ptex[idx][0]);
                vertices.push_back(ptex[idx][1]);
            }
            std::vector<int16_t> indexvec;
            count = faceGmt.triangleCount * 3;
            const int16_t *indices = faceGmt.triangleIndices;
            indexvec.insert(indexvec.end(), indices, indices + count);
            entry::s_ctx->m_pApplication->OnFaceData(fdp, vertices, indexvec);
        }
    }
}
@end
