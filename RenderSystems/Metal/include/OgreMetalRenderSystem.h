/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  -----------------------------------------------------------------------------
*/

#ifndef _OgreMetalRenderSystem_H_
#define _OgreMetalRenderSystem_H_

#include "OgreMetalPrerequisites.h"
#include "OgreMetalPixelFormatToShaderType.h"

#include "OgreRenderSystem.h"
#include "OgreMetalDevice.h"

#import <Metal/MTLRenderCommandEncoder.h>
#import <dispatch/dispatch.h>

namespace Ogre
{
    namespace v1
    {
        class HardwareBufferManager;
    }

    /**
       Implementation of Metal as a rendering system.
    */
    class _OgreMetalExport MetalRenderSystem : public RenderSystem
    {
        struct CachedDepthStencilState
        {
            uint16                      refCount;
            bool                        depthWrite;
            CompareFunction             depthFunc;
            StencilParams               stencilParams;

            id<MTLDepthStencilState>    depthStencilState;

            CachedDepthStencilState() : refCount( 0 ) {}

            bool operator < ( const CachedDepthStencilState &other ) const
            {
                if(   this->depthWrite < other.depthWrite  ) return true;
                if( !(this->depthWrite < other.depthWrite) ) return false;

                if(   this->depthFunc < other.depthFunc  ) return true;
                if( !(this->depthFunc < other.depthFunc) ) return false;

                if(   this->stencilParams < other.stencilParams  ) return true;
                //if( !(this->stencilParams < other.stencilParams) ) return false;

                return false;
            }

            bool operator != ( const CachedDepthStencilState &other ) const
            {
                return this->depthWrite != other.depthWrite ||
                       this->depthFunc != other.depthFunc ||
                       this->stencilParams != other.stencilParams;
            }
        };

        typedef vector<CachedDepthStencilState>::type CachedDepthStencilStateVec;

        bool mInitialized;
        v1::HardwareBufferManager   *mHardwareBufferManager;
        MetalGpuProgramManager      *mShaderManager;
        MetalProgramFactory         *mMetalProgramFactory;

        ConfigOptionMap mOptions;

        MetalPixelFormatToShaderType mPixelFormatToShaderType;

        __unsafe_unretained id<MTLBuffer>   mIndirectBuffer;
        unsigned char                       *mSwIndirectBufferPtr;
        CachedDepthStencilStateVec          mDepthStencilStates;
        MetalHlmsPso const                  *mPso;
        HlmsComputePso const                *mComputePso;
        
        bool mStencilEnabled;
        uint32_t mStencilRefValue;

        //For v1 rendering.
        v1::IndexData       *mCurrentIndexBuffer;
        v1::VertexData      *mCurrentVertexBuffer;
        MTLPrimitiveType    mCurrentPrimType;

        //TODO: AutoParamsBuffer probably belongs to MetalDevice (because it's per device?)
        typedef vector<ConstBufferPacked*>::type ConstBufferPackedVec;
        ConstBufferPackedVec    mAutoParamsBuffer;
        size_t                  mAutoParamsBufferIdx;
        uint8                   *mCurrentAutoParamsBufferPtr;
        size_t                  mCurrentAutoParamsBufferSpaceLeft;
        size_t                  mHistoricalAutoParamsSize[60];

        struct Uav
        {
            TexturePtr      texture;
            id<MTLTexture>  textureName;
            UavBufferPacked *buffer;
            size_t          offset;
            //size_t          sizeBytes;

            Uav() : textureName( 0 ), buffer( 0 ), offset( 0 ) {}
        };

        Uav             mUavs[64];
        /// In range [0; 64]; note that a user may use
        /// mUavs[0] & mUavs[2] leaving mUavs[1] empty.
        /// and still mMaxUavIndexPlusOne = 3.
        uint8           mMaxModifiedUavPlusOne;
        bool            mUavsDirty;

        uint8           mNumMRTs;
        MetalRenderTargetCommon     *mCurrentColourRTs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        MetalDepthBuffer            *mCurrentDepthBuffer;
        MetalDevice                 *mActiveDevice;
        __unsafe_unretained id<MTLRenderCommandEncoder> mActiveRenderEncoder;

        MetalDevice             mDevice;
        dispatch_semaphore_t    mMainGpuSyncSemaphore;
        bool                    mMainSemaphoreAlreadyWaited;
        bool                    mBeginFrameOnceStarted;

        void setActiveDevice( MetalDevice *device );
        void createRenderEncoder(void);

        id<MTLDepthStencilState> getDepthStencilState( HlmsPso *pso );
        void removeDepthStencilState( HlmsPso *pso );

        void cleanAutoParamsBuffers(void);

    public:
        MetalRenderSystem();
        virtual ~MetalRenderSystem();

        virtual void shutdown(void);

        virtual const String& getName(void) const;
        virtual const String& getFriendlyName(void) const;
        virtual ConfigOptionMap& getConfigOptions(void) { return mOptions; }
        virtual void setConfigOption(const String &name, const String &value) {}

        virtual HardwareOcclusionQuery* createHardwareOcclusionQuery(void);

        virtual String validateConfigOptions(void)  { return BLANKSTRING; }

        virtual RenderSystemCapabilities* createRenderSystemCapabilities(void) const;

        virtual void reinitialise(void);

        virtual RenderWindow* _initialise(bool autoCreateWindow, const String& windowTitle = "OGRE Render Window");

        virtual RenderWindow* _createRenderWindow( const String &name,
                                                   unsigned int width, unsigned int height,
                                                   bool fullScreen,
                                                   const NameValuePairList *miscParams = 0);

        virtual MultiRenderTarget* createMultiRenderTarget(const String & name);

        virtual String getErrorDescription(long errorNumber) const;

        virtual void _useLights(const LightList& lights, unsigned short limit);
        virtual void _setWorldMatrix(const Matrix4 &m);
        virtual void _setViewMatrix(const Matrix4 &m);
        virtual void _setProjectionMatrix(const Matrix4 &m);

        virtual void _setSurfaceParams( const ColourValue &ambient,
                                const ColourValue &diffuse, const ColourValue &specular,
                                const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking = TVC_NONE );
        virtual void _setPointSpritesEnabled(bool enabled);
        virtual void _setPointParameters(Real size, bool attenuationEnabled,
            Real constant, Real linear, Real quadratic, Real minSize, Real maxSize);

        virtual void queueBindUAV( uint32 slot, TexturePtr texture,
                                           ResourceAccess::ResourceAccess access = ResourceAccess::ReadWrite,
                                           int32 mipmapLevel = 0, int32 textureArrayIndex = 0,
                                           PixelFormat pixelFormat = PF_UNKNOWN );
        virtual void queueBindUAV( uint32 slot, UavBufferPacked *buffer,
                                   ResourceAccess::ResourceAccess access = ResourceAccess::ReadWrite,
                                   size_t offset = 0, size_t sizeBytes = 0 );
        virtual void clearUAVs(void);
        virtual void flushUAVs(void);

        virtual void _bindTextureUavCS( uint32 slot, Texture *texture,
                                        ResourceAccess::ResourceAccess access,
                                        int32 mipmapLevel, int32 textureArrayIndex,
                                        PixelFormat pixelFormat );
        virtual void _setTextureCS( uint32 slot, bool enabled, Texture *texPtr );
        virtual void _setHlmsSamplerblockCS( uint8 texUnit, const HlmsSamplerblock *samplerblock );

        virtual void _setTexture(size_t unit, bool enabled,  Texture *texPtr);

        virtual void _setTextureCoordSet(size_t unit, size_t index);
        virtual void _setTextureCoordCalculation(size_t unit, TexCoordCalcMethod m,
                                                 const Frustum* frustum = 0);
        virtual void _setTextureBlendMode(size_t unit, const LayerBlendModeEx& bm);
        virtual void _setTextureMatrix(size_t unit, const Matrix4& xform);

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer );

        virtual void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso );
        virtual void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso );

        virtual DepthBuffer* _createDepthBufferFor( RenderTarget *renderTarget,
                                                    bool exactMatchFormat );

        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );

        /// See VaoManager::waitForTailFrameToFinish
        virtual void _waitForTailFrameToFinish(void);
        virtual bool _willTailFrameStall(void);

        virtual void _beginFrameOnce(void);
        virtual void _endFrameOnce(void);

        virtual void _beginFrame(void);
        virtual void _endFrame(void);
        virtual void _setViewport(Viewport *vp);

        virtual void _hlmsPipelineStateObjectCreated( HlmsPso *newPso );
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso );
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock );
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block );

        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock );
        virtual void _setPipelineStateObject( const HlmsPso *pso );
        virtual void _setComputePso( const HlmsComputePso *pso );

        virtual VertexElementType getColourVertexElementType(void) const;
        virtual void _convertProjectionMatrix( const Matrix4& matrix, Matrix4& dest,
                                               bool forGpuProgram = false);
        virtual Real getRSDepthRange(void) const;
        virtual void _makeProjectionMatrix( Real left, Real right, Real bottom, Real top,
                                            Real nearPlane, Real farPlane, Matrix4& dest,
                                            bool forGpuProgram = false );
        virtual void _makeProjectionMatrix( const Radian& fovy, Real aspect, Real nearPlane,
                                            Real farPlane, Matrix4& dest, bool forGpuProgram = false );
        virtual void _makeOrthoMatrix( const Radian& fovy, Real aspect, Real nearPlane, Real farPlane,
                                       Matrix4& dest, bool forGpuProgram = false);
        virtual void _applyObliqueDepthProjection( Matrix4& matrix, const Plane& plane,
                                                   bool forGpuProgram );

        virtual void _dispatch( const HlmsComputePso &pso );

        virtual void _setVertexArrayObject( const VertexArrayObject *vao );

        virtual void _render( const CbDrawCallIndexed *cmd );
        virtual void _render( const CbDrawCallStrip *cmd );
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulated( const CbDrawCallStrip *cmd );

        virtual void _setRenderOperation( const v1::CbRenderOp *cmd );
        virtual void _render( const v1::CbDrawCallIndexed *cmd );
        virtual void _render( const v1::CbDrawCallStrip *cmd );
        virtual void _render( const v1::RenderOperation &op );

        virtual void bindGpuProgramParameters(GpuProgramType gptype,
            GpuProgramParametersSharedPtr params, uint16 variabilityMask);
        virtual void bindGpuProgramPassIterationParameters(GpuProgramType gptype);

        virtual void clearFrameBuffer(unsigned int buffers,
            const ColourValue& colour = ColourValue::Black,
            Real depth = 1.0f, unsigned short stencil = 0);
        virtual void discardFrameBuffer( unsigned int buffers );

        virtual Real getHorizontalTexelOffset(void);
        virtual Real getVerticalTexelOffset(void);
        virtual Real getMinimumDepthInputValue(void);
        virtual Real getMaximumDepthInputValue(void);

        virtual void _setRenderTarget(RenderTarget *target, uint8 viewportRenderTargetFlags);
        virtual void _notifyCompositorNodeSwitchedRenderTarget( RenderTarget *previousTarget );
        virtual void preExtraThreadsStarted();
        virtual void postExtraThreadsStarted();
        virtual void registerThread();
        virtual void unregisterThread();
        virtual unsigned int getDisplayMonitorCount() const     { return 1; }

        virtual const PixelFormatToShaderType* getPixelFormatToShaderType(void) const;

        virtual void beginProfileEvent( const String &eventName );
        virtual void endProfileEvent( void );
        virtual void markProfileEvent( const String &event );

        virtual bool hasAnisotropicMipMapFilter() const         { return true; }

        virtual void setClipPlanesImpl(const PlaneList& clipPlanes);
        virtual void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities* caps,
                                                             RenderTarget* primary );
        virtual void updateCompositorManager( CompositorManager2 *compositorManager,
                                              SceneManagerEnumerator &sceneManagers,
                                              HlmsManager *hlmsManager );

        MetalDevice* getActiveDevice(void)                      { return mActiveDevice; }
        MetalProgramFactory* getMetalProgramFactory(void)       { return mMetalProgramFactory; }

        void _notifyActiveEncoderEnded(void);
        void _notifyActiveComputeEnded(void);
        void _notifyDeviceStalled(void);
    };
}

#endif