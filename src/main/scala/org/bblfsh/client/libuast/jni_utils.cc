#include "jni_utils.h"
#include "memtracker.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Type signatures; to get the signature of all methods from a class do:
// javap -s -p SomeClass.class
// To create a Java version of this module from the same codebase, #ifdefs should
// be sprinkled here to get the equivalent Java types.
const char *TYPE_STR = "Ljava/lang/String;";
const char *TYPE_OPTION = "Lscala/Option;";
const char *TYPE_SEQ = "Lscala/collection/Seq;";
const char *TYPE_MAP = "Lscala/collection/immutable/Map;";

// Method signatures. Same as above: use javap to get them from a .class
const char *METHOD_SEQ_APPLY = "(I)Ljava/lang/Object;";
const char *METHOD_MAP_APPLY = "(Ljava/lang/Object;)Ljava/lang/Object;";
const char *METHOD_MUTLIST_TOLIST = "()Lscala/collection/immutable/List;";
const char *METHOD_MUTLIST_TOIMMLIST = "()Lscala/collection/immutable/List;";
const char *METHOD_LIST_INIT = "()V";
const char *METHOD_LIST_PLUSEQ = "(Ljava/lang/Object;)Lscala/collection/mutable/MutableList;";
const char *METHOD_OPTION_GET = "()Ljava/lang/Object;";
const char *METHOD_SEQ_SORTED = "(Lscala/math/Ordering;)Ljava/lang/Object;";
const char *METHOD_MAP_KEYS = "()Lscala/collection/GenIterable;";

// Class fully qualified names
const char *CLS_NODE_EXT = "org/bblfsh/client/NodeExt";
const char *CLS_NODE = "gopkg/in/bblfsh/sdk/v1/uast/generated/Node";
const char *CLS_POSITION = "gopkg/in/bblfsh/sdk/v1/uast/generated/Position";
const char *CLS_ROLE = "gopkg/in/bblfsh/sdk/v1/uast/generated/Role";
const char *CLS_OPTION = "scala/Option";
const char *CLS_SEQ = "scala/collection/Seq";
const char *CLS_MAP = "scala/collection/Map";
const char *CLS_LIST = "scala/collection/immutable/List";
const char *CLS_MUTLIST = "scala/collection/mutable/MutableList";
const char *CLS_ITERABLE = "scala/collection/GenIterable";

const char *CLS_JAVA_MAP = "java/util/TreeMap";
const char *CLS_JAVA_ARR = "java/util/ArrayList";
const char *CLS_JAVA_STR = "java/lang/String";

extern JavaVM *jvm;
extern MemTracker memTracker;

//// JNI helpers
JNIEnv *getJNIEnv() {
  JNIEnv *pEnv = NULL;

  switch (jvm->GetEnv((void **)&pEnv, JNI_VERSION_1_8))
  {
    case JNI_OK:
      // Thread is ready to use, nothing to do
      break;

    case JNI_EDETACHED:
      // Thread is detached, need to attach
      jvm->AttachCurrentThread((void **)&pEnv, NULL);
      break;
  }

  return pEnv;
}

const char *AsNativeStr(jstring jstr) {
  if (!jstr) {
    return NULL;
  }

  JNIEnv *env = getJNIEnv();
  if (!env) {
    return NULL;
  }

  const char *tmp = env->GetStringUTFChars(jstr, 0);
  if (env->ExceptionOccurred() || !tmp) {
    return NULL;
  }

  // str must be copied to deref the java string before return
  const char *cstr = strdup(tmp);
  memTracker.AddUtilsNode((void*)cstr);

  env->ReleaseStringUTFChars(jstr, tmp);
  if (env->ExceptionOccurred()) {
    return NULL;
  }

  return cstr;
}

jobject *ToObjectPtr(jobject *object) {
  jobject *copy = (jobject*)malloc(sizeof(jobject));
  memcpy(copy, object, sizeof(jobject));
  memTracker.AddUtilsNode((void*)copy);
  return copy;
}

static jmethodID MethodID(JNIEnv *env, const char *method, const char *signature,
                   const char *className, const jobject *object) {
  jclass cls = env->FindClass(className);
  if (env->ExceptionOccurred() || !cls) {
    return NULL;
  }

  jmethodID mId = env->GetMethodID(cls, method, signature);
  if (env->ExceptionOccurred()) {
    return NULL;
  }

  return mId;
}

static jfieldID FieldID(JNIEnv *env, const char *className, const char *field,
                        const char *typeSignature) {
  jclass cls = env->FindClass(className);
  if (env->ExceptionOccurred() || !cls)
    return NULL;

  // Note: printing the type from Scala to find the type needed for GetFieldID
  // third argument using getClass.getName sometimes return objects different
  // from the ones needed for the signature. To find the right type to use do
  // this from Scala: (instance).getClass.getDeclaredField("fieldName")
  jfieldID fId = env->GetFieldID(cls, field, typeSignature);
  if (env->ExceptionOccurred() || !fId)
    return NULL;

  return fId;
}

jint IntMethod(JNIEnv *env, const char *method, const char *signature, const char *className,
               const jobject *object) {

  jmethodID mId = MethodID(env, method, signature, className, object);
  if (env->ExceptionOccurred() || !mId)
    //TODO: report exception and ExceptionClear() it!
    return 0;

  jint res = env->CallIntMethod(*object, mId);
  if (env->ExceptionOccurred()) {
    return 0;
  }

  return res;
}

jlong LongMethod(JNIEnv *env, const char *method, const char *signature, const char *className,
               const jobject *object) {
  jmethodID mId = MethodID(env, method, signature, className, object);
  if (env->ExceptionOccurred() || !mId)
    return 0;

  jlong res = env->CallLongMethod(*object, mId);
  if (env->ExceptionOccurred())
    return 0;

  return res;
}

jboolean BooleanMethod(JNIEnv *env, const char *method, const char *signature,
                       const char *className, const jobject *object) {
  jmethodID mId = MethodID(env, method, signature, className, object);
  if (env->ExceptionOccurred() || !mId)
    return false;

  jboolean res = env->CallBooleanMethod(*object, mId);
  if (env->ExceptionOccurred())
    return false;

  return res;
}

jobject ObjectMethod(JNIEnv *env, const char *method, const char *signature,
                     const char *className, const jobject *object, ...) {
  jmethodID mId = MethodID(env, method, signature, className, object);
  if (env->ExceptionOccurred() || !mId)
    return NULL;

  va_list varargs;
  va_start(varargs, object);
  jobject res = env->CallObjectMethodV(*object, mId, varargs);
  va_end(varargs);
  if (env->ExceptionOccurred() || !res)
    return NULL;

  return res;
}

jobject ObjectField(JNIEnv *env, const char *className, const jobject *obj,
                    const char *field, const char *typeSignature) {
  jfieldID valueId = FieldID(env, className, field, typeSignature);
  if (env->ExceptionOccurred() || !valueId) {
    return NULL;
  }

  jobject value = env->GetObjectField(*obj, valueId);
  if (env->ExceptionOccurred() || !value) {
    return NULL;
  }

  return value;
}

jint IntField(JNIEnv *env, const char *className, const jobject *obj, const char *field) {
  jfieldID valueId = FieldID(env, className, field, "I");
  if (env->ExceptionOccurred() || !valueId)
    return 0;

  jint value = env->GetIntField(*obj, valueId);
  if (env->ExceptionOccurred() || !value)
    return 0;

  return value;
}

jobject NewJavaObject(JNIEnv *env, const char *className, const char *initSign, ...) {
  jclass cls = env->FindClass(className);
  if (env->ExceptionOccurred() || !cls) {
    return NULL;
  }

  jmethodID initId = env->GetMethodID(cls, "<init>", initSign);
  if (env->ExceptionOccurred() || !initId) {
    return NULL;
  }

  va_list varargs;
  va_start(varargs, initSign);
  jobject instance = env->NewObjectV(cls, initId, varargs);
  va_end(varargs);
  if (env->ExceptionOccurred() || !instance) {
    return NULL;
  }

  return instance;
}

const char *ReadStr(const jobject *node, const char *property) {
  JNIEnv *env = getJNIEnv();
  if (!env)
    return NULL;

  jclass cls = env->FindClass(CLS_NODE);
  if (env->ExceptionOccurred() || !cls)
    return NULL;

  jstring jvstr = (jstring)ObjectField(env, CLS_NODE, node, property, TYPE_STR);
  if (env->ExceptionOccurred() || !jvstr) {
    return NULL;
  }

  return AsNativeStr(jvstr);
}

int ReadLen(const jobject *node, const char *property) {
  JNIEnv *env = getJNIEnv();
  if (!env)
    return 0;

  jclass cls = env->FindClass(CLS_NODE);
  if (env->ExceptionOccurred() || !cls)
    return 0;

  jobject childSeq = ObjectField(env, CLS_NODE, node, property, TYPE_SEQ);
  if (env->ExceptionOccurred() || !childSeq) {
    return 0;
  }

  return (int)IntMethod(env, "length", "()I", CLS_SEQ, &childSeq);
}

void ThrowException(const char* message) {
  JNIEnv *env = getJNIEnv();
  if (!env)
    return;

  jclass jcls = env->FindClass("java/lang/Exception");
  if (env->ExceptionCheck() == JNI_TRUE)
    return;

  env->ThrowNew(jcls, message);
}
