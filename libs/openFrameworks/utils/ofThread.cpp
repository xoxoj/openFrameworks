#include "ofThread.h"
#include "ofLog.h"
#include "ofUtils.h"

#ifdef TARGET_ANDROID
#include <jni.h>
#include "ofxAndroidUtils.h"
#endif


//-------------------------------------------------
ofThread::ofThread()
:threadRunning(false)
,threadDone(true)
,mutexBlocks(true)
,name(""){
}


//-------------------------------------------------
ofThread::~ofThread(){
}


//-------------------------------------------------
bool ofThread::isThreadRunning() const{
    return threadRunning;
}


//-------------------------------------------------
std::thread::id ofThread::getThreadId() const{
	return thread.get_id();
}


//-------------------------------------------------
std::string ofThread::getThreadName() const{
	return name;
}

//-------------------------------------------------
void ofThread::setThreadName(const std::string & name){
	this->name = name;
}

//-------------------------------------------------
void ofThread::startThread(bool mutexBlocks){
	if(threadRunning || !threadDone){
		ofLogWarning("ofThread") << "- name: " << getThreadName() << " - Cannot start, thread already running.";
		return;
	}

    threadDone = false;
    threadRunning = true;
    this->mutexBlocks = mutexBlocks;

	thread = std::thread(std::bind(&ofThread::run,this));
}


//-------------------------------------------------
void ofThread::startThread(bool mutexBlocks, bool verbose){
    ofLogWarning("ofThread") << "- name: " << getThreadName() << " - Calling startThread with verbose is deprecated.";
    startThread(mutexBlocks);
}


//-------------------------------------------------
bool ofThread::lock(){
	if(mutexBlocks){
		mutex.lock();
	}else{
		if(!mutex.try_lock()){
			return false; // mutex is locked, tryLock failed
		}
	}
	return true;
}


//-------------------------------------------------
void ofThread::unlock(){
	mutex.unlock();
}


//-------------------------------------------------
void ofThread::stopThread(){
    threadRunning = false;
}


//-------------------------------------------------
void ofThread::waitForThread(bool callStopThread, long milliseconds){
	if(!threadDone){
		// tell thread to stop
		if(callStopThread){
            stopThread(); // signalled to stop
		}

		// wait for the thread to finish
        if(isCurrentThread()){
			return; // waitForThread should only be called outside thread
		}

        if (INFINITE_JOIN_TIMEOUT == milliseconds){
        	std::unique_lock<std::mutex> lck(mutex);
        	timeoutJoin.wait(lck);
        }else{
            // Wait for "joinWaitMillis" milliseconds for thread to finish
        	std::unique_lock<std::mutex> lck(mutex);
            if(timeoutJoin.wait_for(lck,std::chrono::milliseconds(milliseconds))==std::cv_status::timeout){
				// unable to completely wait for thread
            }
        }
    }
}


//-------------------------------------------------
void ofThread::sleep(long milliseconds){
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}


//-------------------------------------------------
void ofThread::yield(){
	std::this_thread::yield();
}


//-------------------------------------------------
bool ofThread::isCurrentThread() const{
    return std::this_thread::get_id() == thread.get_id();
}


//-------------------------------------------------
std::thread & ofThread::getNativeThread(){
	return thread;
}


//-------------------------------------------------
const std::thread & ofThread::getNativeThread() const{
	return thread;
}


//-------------------------------------------------
bool ofThread::isMainThread(){
    return false;
}


//-------------------------------------------------
std::thread * ofThread::getCurrentNativeThread(){
	return nullptr;
}


//-------------------------------------------------
void ofThread::threadedFunction(){
	ofLogWarning("ofThread") << "- name: " << getThreadName() << " - Override ofThread::threadedFunction() in your ofThread subclass.";
}


//-------------------------------------------------
void ofThread::run(){
#ifdef TARGET_ANDROID
	JNIEnv * env;
	jint attachResult = ofGetJavaVMPtr()->AttachCurrentThread(&env,NULL);
	if(attachResult!=0){
		ofLogWarning() << "couldn't attach new thread to java vm";
	}
#endif
	// user function
    // should loop endlessly.
	try{
		threadedFunction();
	}catch(const std::exception& exc){
		ofLogFatalError("ofThreadErrorLogger::exception") << exc.what();
	}catch(...){
		ofLogFatalError("ofThreadErrorLogger::exception") << "Unknown exception.";
	}
	thread.detach();
#ifdef TARGET_ANDROID
	attachResult = ofGetJavaVMPtr()->DetachCurrentThread();
#endif
    std::unique_lock<std::mutex> lck(mutex);
	threadRunning = false;
	threadDone = true;
	timeoutJoin.notify_all();
}
