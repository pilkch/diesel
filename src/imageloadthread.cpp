// Spitfire headers
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "imageloadthread.h"
#include "util.h"

namespace diesel
{
  cFolderLoadRequest::cFolderLoadRequest(const string_t& _sFolderPath, IMAGE_SIZE _imageSize) :
    sFolderPath(_sFolderPath),
    imageSize(_imageSize)
  {
  }


  // ** cImageLoadThread

  cImageLoadThread::cImageLoadThread(cImageLoadHandler& _handler) :
    spitfire::util::cThread(soAction, "cImageLoadThread::cThread"),
    handler(_handler),
    soAction("cImageLoadThread::soAction"),
    requestQueue(soAction)
  {
  }

  void cImageLoadThread::Start()
  {
    // Start
    Run();
  }

  void cImageLoadThread::StopSoon()
  {
    StopThreadSoon();
  }

  void cImageLoadThread::StopNow()
  {
    StopThreadNow();
  }

  void cImageLoadThread::LoadFolder(const string_t& sFolderPath, IMAGE_SIZE imageSize)
  {
    // Add an event to the queue
    requestQueue.AddItemToBack(new cFolderLoadRequest(sFolderPath, imageSize));
  }

  void cImageLoadThread::StopLoading()
  {
    loadingProcessInterface.SetStop();

    // Remove all the remaining events
    ClearEventQueue();
  }

  void cImageLoadThread::ClearEventQueue()
  {
    // Remove and delete all events on the queue
    while (true) {
      cFolderLoadRequest* pEvent = requestQueue.RemoveItemFromFront();
      if (pEvent == nullptr) break;

      spitfire::SAFE_DELETE(pEvent);
    }
  }

  void cImageLoadThread::ThreadFunction()
  {
    LOG<<"cImageLoadThread::ThreadFunction"<<std::endl;

    while (true) {
      //LOG<<"cImageLoadThread::ThreadFunction Loop"<<std::endl;
      soAction.WaitTimeoutMS(1000);

      if (IsToStop()) break;

      //LOG<<"cImageLoadThread::ThreadFunction Loop getting event"<<std::endl;
      cFolderLoadRequest* pRequest = requestQueue.RemoveItemFromFront();
      if (pRequest != nullptr) {



        // Collect a list of the files in this directory
        std::list<string_t> files;

        for (spitfire::filesystem::cFolderIterator iter(pRequest->sFolderPath); iter.IsValid(); iter.Next()) {
          const string_t sFilePath = iter.GetFullPath();
          if (iter.IsFolder()) {
            // Tell the handler that we found a folder
            handler.OnFolderFound(sFilePath);
          } else {
            // Tell the handler that we found an image file
            handler.OnImageLoading(sFilePath);

            files.push_back(sFilePath);
          }
        }

        // Now go back and actually load the images
        const std::list<string_t>::const_iterator iterEnd = files.end();
        for (std::list<string_t>::const_iterator iter = files.begin(); iter != iterEnd; iter++) {

          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          const string_t sFilePath = *iter;
          /*
          if (sFilePath == raw) sFilePath = imageCacheManager.GetOrCreateDNGForRawFile(sFilePath);
          else if (sFilePath == jpg) ...

          if (jpg) {
            Create thumbnail from jpg
            Load and return
          } else {
            // If we have a raw file then we need to convert it and load
            if (raw) sFilePath = imageCacheManager.GetOrCreateDNGForRawFile(sFilePath);

            Create thumbnail from dng
            Load and return
          }

          const string_t sThumbnailFilePath = imageCacheManager.GetOrCreateThumbnailForFile(sFilePath, pRequest->imageSize);*/

          voodoo::cImage* pImage = new voodoo::cImage;

          pImage->LoadFromFile(sFilePath);

          // Loading the image can take a while so we need to check again if we should stop
          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          // Notify the handler
          if (pImage->IsValid()) handler.OnImageLoaded(sFilePath, pRequest->imageSize, pImage);
          else {
            handler.OnImageError(sFilePath, pRequest->imageSize);

            // Delete the image
            delete pImage;
          }
        }

        //LOG<<"cImageLoadThread::ThreadFunction Loop deleting event"<<std::endl;
        spitfire::SAFE_DELETE(pRequest);
      } else {
        // If the queue is empty then we know that there are no more actions and it is safe to reset our stop loading signal object
        loadingProcessInterface.Reset();
      }

      // Try to avoid hogging the CPU
      spitfire::util::SleepThisThreadMS(1);
      spitfire::util::YieldThisThread();
    }

    // Remove any further events because we don't care any more
    ClearEventQueue();

    LOG<<"cImageLoadThread::ThreadFunction returning"<<std::endl;
  }
}
