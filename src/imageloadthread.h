#ifndef DIESEL_IMAGELOADTHREAD_H
#define DIESEL_IMAGELOADTHREAD_H

// libvoodoomm headers
#include <libvoodoomm/cImage.h>

// Spitfire headers
#include <spitfire/util/process.h>
#include <spitfire/util/queue.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  // Supported files
  //
  // Diesel handles raw, dng and image files
  // Raw files are nef, crw, etc.
  // Dng files are dng
  // Image files are jpg, png, etc.
  //
  // Diesel will convert a raw file to dng if a dng file doesn't exist already
  // It will then create full sized images and thumbnails from the dng if it exists, or the image files if no raw or dng files exist and only the image files are available
  //


  class cFolderLoadThumbnailsRequest
  {
  public:
    cFolderLoadThumbnailsRequest(const string_t& sFolderPath);

    string_t sFolderPath;
  };

  class cFileLoadFullHighPriorityRequest
  {
  public:
    cFileLoadFullHighPriorityRequest(const string_t& sFileNameNoExtension);

    string_t sFileNameNoExtension;
  };


  // ** cPhoto

  class cPhoto
  {
  public:
    cPhoto();

    string_t sFilePath;

    // NOTE: The camera may have created a raw, dng, image, or a combination of these.  The user may also have converted to dng or exported an image
    bool bHasRaw; // Nef, crw, etc.
    bool bHasDNG;
    bool bHasImage; // Jpg, png, etc.
  };

  inline cPhoto::cPhoto() :
    bHasRaw(false),
    bHasDNG(false),
    bHasImage(false)
  {
  }


  class cImageLoadThread;

  class cImageLoadHandler
  {
  public:
    friend class cImageLoadThread;

    virtual ~cImageLoadHandler() {}

  private:
    virtual void OnFolderFound(const string_t& sFolderName) = 0;
    virtual void OnFileFound(const string_t& sFileNameNoExtension) = 0;
    virtual void OnImageLoaded(const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, voodoo::cImage* pImage) = 0;
    virtual void OnImageError(const string_t& sFileNameNoExtension) = 0;
  };

  class cImageLoadThread : protected spitfire::util::cThread
  {
  public:
    explicit cImageLoadThread(cImageLoadHandler& handler);

    void Start();
    void StopSoon();
    void StopNow();

    void SetMaximumCacheSizeGB(size_t nSizeGB);

    void LoadFolderThumbnails(const string_t& sFolderPath);
    void LoadFileFullHighPriority(const string_t& sFilePath);
    void StopLoading();

  private:
    virtual void ThreadFunction() override;

    void ClearEventQueue();

    bool GetOrCreateDNGForRawFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension, cPhoto& photo);
    string_t GetOrCreateThumbnail(const string_t& sFolderPath, const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, cPhoto& photo);
    void LoadThumbnailImage(const string_t& sThumbnailFilePath, const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize);

    void HandleHighPriorityRequestQueue(const string_t& sFolderPath, std::map<string_t, cPhoto*>& files);

    cImageLoadHandler& handler;

    spitfire::util::cSignalObject soAction;

    spitfire::util::cThreadSafeQueue<cFolderLoadThumbnailsRequest> requestQueue;

    spitfire::util::cThreadSafeQueue<cFileLoadFullHighPriorityRequest> highPriorityRequestQueue;

    class cLoadingProcessInterface : public spitfire::util::cProcessInterface
    {
    public:
      cLoadingProcessInterface();

      virtual bool _IsToStop() const override { return soStopLoading.IsSignalled(); }

      void SetStop() { soStopLoading.Signal(); }
      void Reset() { soStopLoading.Reset(); }

    private:
      spitfire::util::cSignalObject soStopLoading;
    };

    spitfire::util::cMutex mutexMaximumCacheSize;
    size_t nMaximumCacheSizeGB;

    cLoadingProcessInterface loadingProcessInterface; // Signalled by the controller when the the model thread should stop loading files
  };


  // Inlines

  inline cImageLoadThread::cLoadingProcessInterface::cLoadingProcessInterface() :
    soStopLoading(TEXT("cImageLoadThread::cLoadingProcessInterface_soStopLoading"))
  {
  }
}

#endif // DIESEL_IMAGELOADTHREAD_H
