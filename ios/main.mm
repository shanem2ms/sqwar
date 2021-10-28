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

@interface AVDelegate : NSObject<AVCaptureDataOutputSynchronizerDelegate>
{
    AVCaptureDepthDataOutput *m_pDepthOutput;
    AVCaptureVideoDataOutput *m_pVideoOutput;
}
- (id)initWithDepthOutput:(AVCaptureDepthDataOutput *)pDataOutput withVideo:(AVCaptureVideoDataOutput *)pVideoOutput;
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
    AVCaptureSession *m_pSession;
    AVCaptureDepthDataOutput *m_pDepthOutput;
    AVCaptureVideoDataOutput *m_pVideoOutput;
    AVDelegate *m_pAVDelegate;
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

    /*
    m_dataQueue =
        dispatch_queue_create("avqueue", nullptr);
    m_pSession = [[AVCaptureSession alloc] init];
    m_pDepthOutput = [[AVCaptureDepthDataOutput alloc] init];
    m_pVideoOutput = [[AVCaptureVideoDataOutput alloc] init];
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo  completionHandler:^(BOOL granted) {
        if (granted) {
            //self.microphoneConsentState = PrivacyConsentStateGranted;
        }
        else {
            //self.microphoneConsentState = PrivacyConsentStateDenied;
        }
    }];
    */
    m_sessionQueue =
        dispatch_queue_create("sessionqueue", nullptr);
    dispatch_async(m_sessionQueue,  ^(void){
        [self configureARSession];
    });
    
    
    
    return YES;
}

- (void)configureSession
{
    AVCaptureDevice *pDevice = nullptr;
    if (@available(iOS 11.1, *)) {
        AVCaptureDeviceDiscoverySession *captureDevice =
        [AVCaptureDeviceDiscoverySession
         discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInTrueDepthCamera]
         mediaType:AVMediaTypeVideo
         position:AVCaptureDevicePositionFront];
        pDevice = captureDevice.devices.lastObject;
    } else {
        // Fallback on earlier versions
    }
    NSError *perror;
    AVCaptureDeviceInput *pInput = [[AVCaptureDeviceInput alloc] initWithDevice:pDevice error:&perror];
    [m_pSession beginConfiguration];
    [m_pSession setSessionPreset:AVCaptureSessionPreset640x480];
    [m_pSession addInput:pInput];
    [m_pSession addOutput:m_pDepthOutput];
    [m_pSession addOutput:m_pVideoOutput];
    //videoDataOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: Int(kCVPixelFormatType_32BGRA)]
    NSDictionary *rgbOutputSettings = [NSDictionary dictionaryWithObject:
     [NSNumber numberWithInt:kCMPixelFormat_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    [m_pVideoOutput setVideoSettings:rgbOutputSettings];
    [m_pDepthOutput setFilteringEnabled:true];
    AVCaptureConnection *pDepthConnection = [m_pDepthOutput connectionWithMediaType:AVMediaTypeDepthData];
    [pDepthConnection setEnabled:true];
    NSArray<AVCaptureDeviceFormat *> *depthFormats = [[pDevice activeFormat] supportedDepthDataFormats];
    unsigned long cnt = [depthFormats count];
    unsigned long foundIdx = -1;
    int maxwidth = 0;
    for (unsigned long fidx = 0; fidx < cnt; fidx++)
    {
        if (CMFormatDescriptionGetMediaSubType([depthFormats[fidx] formatDescription]) == kCVPixelFormatType_DepthFloat32)
        {
            int w = CMVideoFormatDescriptionGetDimensions([depthFormats[fidx] formatDescription]).width;
            if (w > maxwidth)
            {
                foundIdx = fidx;
                maxwidth = w;
            }
        }
    }
    
    [pDevice lockForConfiguration:&perror];
    [pDevice setActiveDepthDataFormat:depthFormats[foundIdx]];
    [pDevice unlockForConfiguration];
    AVCaptureDataOutputSynchronizer *pOutputSyncronizer =
        [[AVCaptureDataOutputSynchronizer alloc] initWithDataOutputs:@[m_pDepthOutput, m_pVideoOutput]];
    
    m_pAVDelegate = [[AVDelegate alloc] initWithDepthOutput:m_pDepthOutput withVideo:m_pVideoOutput];
    [pOutputSyncronizer setDelegate:m_pAVDelegate queue:m_dataQueue];
    [m_pSession commitConfiguration];
    
    [m_pSession startRunning];
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

@implementation AVDelegate

- (id)initWithDepthOutput:(AVCaptureDepthDataOutput *)pDataOutput withVideo:(AVCaptureVideoDataOutput *)pVideoOutput
{
    self = [super init];
    m_pDepthOutput = pDataOutput;
    m_pVideoOutput = pVideoOutput;
    return self;
}

- (void * _Nullable)extracted:(CVImageBufferRef)imgbufref {
    return CVPixelBufferGetBaseAddress(imgbufref);
}

- (void)dataOutputSynchronizer:(AVCaptureDataOutputSynchronizer *)synchronizer
didOutputSynchronizedDataCollection:(AVCaptureSynchronizedDataCollection *)synchronizedDataCollection
{
    AVCaptureSynchronizedSampleBufferData *pVidSync =
        (AVCaptureSynchronizedSampleBufferData *)[synchronizedDataCollection synchronizedDataForCaptureOutput:m_pVideoOutput];
    AVCaptureSynchronizedDepthData *pSyncData =
        (AVCaptureSynchronizedDepthData *)[synchronizedDataCollection synchronizedDataForCaptureOutput:m_pDepthOutput];
    if (pSyncData.depthDataWasDropped || pVidSync.sampleBufferWasDropped)
        return;
    
    CMSampleBufferRef sbufferref = pVidSync.sampleBuffer;
    CVImageBufferRef imgbufref = CMSampleBufferGetImageBuffer(sbufferref);
    CMFormatDescriptionRef fmtref = CMSampleBufferGetFormatDescription(sbufferref);
    
    AVDepthData *pDepthData = pSyncData.depthData;
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
    CVPixelBufferRef pixelBuffer = pDepthData.depthDataMap;

    if (kCVPixelFormatType_DepthFloat32 == CVPixelBufferGetPixelFormatType(pixelBuffer))
    {
        std::vector<float> pixelData(640*480 + 16);
        std::vector<unsigned char> vidData(640*480*4);
        memcpy(pixelData.data(), mtxarray, sizeof(mtxarray));
        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        void *pBuffer = CVPixelBufferGetBaseAddress(pixelBuffer);
        memcpy(pixelData.data() + 16, pBuffer, pixelData.size() * sizeof(float));
        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferLockBaseAddress(imgbufref, kCVPixelBufferLock_ReadOnly);
        size_t datasize = CVPixelBufferGetDataSize(imgbufref);
        void *pVidBuffer = [self extracted:imgbufref];
        memcpy(vidData.data(), pVidBuffer, vidData.size());
        CVPixelBufferUnlockBaseAddress(imgbufref, kCVPixelBufferLock_ReadOnly);
        sam::Application::DepthDataProps wdp;
        wdp.depthHeight = CVPixelBufferGetWidth(pixelBuffer);
        wdp.depthWidth = CVPixelBufferGetHeight(pixelBuffer);
        wdp.vidHeight = CVPixelBufferGetWidth(imgbufref);
        wdp.vidWidth = CVPixelBufferGetHeight(imgbufref);
        wdp.depthMode = wdp.vidMode = 0;
        entry::s_ctx->m_pApplication->OnDepthBuffer(vidData, pixelData, wdp);
    }
    
}

@end

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
        std::vector<unsigned char> vidData(datasize);
        memcpy(vidData.data(), pVidBuffer, vidData.size());
        CVPixelBufferUnlockBaseAddress(imgBuffer, kCVPixelBufferLock_ReadOnly);
    
        datasize = CVPixelBufferGetDataSize(depthBuffer);
        std::vector<float> depthData(datasize / sizeof(float) +  16);
        memcpy(depthData.data(), mtxarray, sizeof(mtxarray));
        
        CVPixelBufferLockBaseAddress(depthBuffer, kCVPixelBufferLock_ReadOnly);
        void *pDepthBuffer = CVPixelBufferGetBaseAddress(depthBuffer);
        memcpy(depthData.data() + 16, pDepthBuffer, depthData.size());
        CVPixelBufferUnlockBaseAddress(depthBuffer, kCVPixelBufferLock_ReadOnly);
        sam::Application::DepthDataProps wdp;
        wdp.depthHeight = CVPixelBufferGetWidth(depthBuffer);
        wdp.depthWidth = CVPixelBufferGetHeight(depthBuffer);
        wdp.vidHeight = CVPixelBufferGetWidth(imgBuffer);
        wdp.vidWidth = CVPixelBufferGetHeight(imgBuffer);
        wdp.depthMode = wdp.vidMode = 0;
        entry::s_ctx->m_pApplication->OnDepthBuffer(vidData, depthData, wdp);
    }
}
@end
