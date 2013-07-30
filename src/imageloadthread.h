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
  class cFolderLoadRequest
  {
  public:
    cFolderLoadRequest(const string_t& sFolderPath, IMAGE_SIZE imageSize);

    string_t sFolderPath;
    IMAGE_SIZE imageSize;
  };

  class cImageLoadThread;

  class cImageLoadHandler
  {
  public:
    friend class cImageLoadThread;

    virtual ~cImageLoadHandler() {}

  private:
    virtual void OnFolderFound(const string_t& sFolderPath) = 0;
    virtual void OnImageLoading(const string_t& sFilePath) = 0;
    virtual void OnImageLoaded(const string_t& sFilePath, IMAGE_SIZE imageSize, voodoo::cImage* pImage) = 0;
    virtual void OnImageError(const string_t& sFilePath, IMAGE_SIZE imageSize) = 0;
  };

  class cImageLoadThread : protected spitfire::util::cThread
  {
  public:
    explicit cImageLoadThread(cImageLoadHandler& handler);

    void Start();
    void StopSoon();
    void StopNow();

    void LoadFolder(const string_t& sFolderPath, IMAGE_SIZE imageSize);
    void StopLoading();

  private:
    virtual void ThreadFunction() override;

    cImageLoadHandler& handler;

    spitfire::util::cSignalObject soAction;

    spitfire::util::cThreadSafeQueue<cFolderLoadRequest> requestQueue;

    cImageCacheManager imageCacheManager;


    class cLoadingProcessInterface : public spitfire::util::cProcessInterface
    {
    public:
      cLoadingProcessInterface();

      override virtual bool IsToStop() const { return soStopLoading.IsSignalled(); }

      void SetStop() { soStopLoading.Signal(); }
      void Reset() { soStopLoading.Reset(); }

    private:
      spitfire::util::cSignalObject soStopLoading;
    };

    cLoadingProcessInterface loadingProcessInterface; // Signalled by the controller when the the model thread should stop loading files
  };


  // Inlines

  inline cImageLoadThread::cLoadingProcessInterface::cLoadingProcessInterface() :
    soStopLoading("cImageLoadThread::cLoadingProcessInterface_soStopLoading")
  {
  }
}

#endif // DIESEL_IMAGELOADTHREAD_H
