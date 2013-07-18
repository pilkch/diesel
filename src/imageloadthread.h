#ifndef DIESEL_IMAGELOADTHREAD_H
#define DIESEL_IMAGELOADTHREAD_H

// libvoodoomm headers
#include <libvoodoomm/cImage.h>

// Spitfire headers
#include <spitfire/util/queue.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  class cImageLoadThread;

  enum class IMAGE_SIZE {
    SMALL,
    MEDIUM,
    FULL
  };

  class cImageLoadRequest
  {
  public:
    cImageLoadRequest(const string_t& sFilePath, IMAGE_SIZE imageSize);

    string_t sFilePath;
    IMAGE_SIZE imageSize;
  };

  class cImageLoadThread;

  class cImageLoadHandler
  {
  public:
    friend class cImageLoadThread;

    virtual ~cImageLoadHandler() {}

  private:
    virtual void OnImageError(const string_t& sFilePath, IMAGE_SIZE imageSize) = 0;
    virtual void OnImageLoaded(const string_t& sFilePath, IMAGE_SIZE imageSize, voodoo::cImage* pImage) = 0;
  };

  class cImageLoadThread : protected spitfire::util::cThread
  {
  public:
    explicit cImageLoadThread(cImageLoadHandler& handler);

    void Start();
    void StopSoon();
    void StopNow();

    void LoadThumbnail(const string_t& sFilePath, IMAGE_SIZE imageSize);
    void ClearRequestQueue();

  private:
    virtual void ThreadFunction() override;

    cImageLoadHandler& handler;

    spitfire::util::cSignalObject soAction;

    spitfire::util::cThreadSafeQueue<cImageLoadRequest> requestQueue;
  };
}

#endif // DIESEL_IMAGELOADTHREAD_H
