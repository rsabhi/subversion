/**
 * @copyright
 * ====================================================================
 * Copyright (c) 2007 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 * @endcopyright
 *
 * @file ConflictResolverCallback.cpp
 * @brief Implementation of the class ConflictResolverCallback.
 */

#include "svn_error.h"

#include "../include/org_tigris_subversion_javahl_ConflictResolverCallback_Result.h"
#include "JNIUtil.h"
#include "EnumMapper.h"
#include "ConflictResolverCallback.h"

ConflictResolverCallback::ConflictResolverCallback(jobject jconflictResolver)
{
  m_conflictResolver = jconflictResolver;
}

ConflictResolverCallback::~ConflictResolverCallback()
{
  if (m_conflictResolver != NULL)
    {
      JNIEnv *env = JNIUtil::getEnv();
      env->DeleteGlobalRef(m_conflictResolver);
    }
}

ConflictResolverCallback *
ConflictResolverCallback::makeCConflictResolverCallback(jobject jconflictResolver)
{
  if (jconflictResolver == NULL)
    return NULL;

  JNIEnv *env = JNIUtil::getEnv();

  // Sanity check that the object implements the ConflictResolverCallback
  // Java interface.
  jclass clazz = env->FindClass(JAVA_PACKAGE "/ConflictResolverCallback");
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  if (!env->IsInstanceOf(jconflictResolver, clazz))
    {
      env->DeleteLocalRef(clazz);
      return NULL;
    }
  env->DeleteLocalRef(clazz);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // Retain a global reference to our Java peer.
  jobject myListener = env->NewGlobalRef(jconflictResolver);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // Create the peer.
  return new ConflictResolverCallback(myListener);
}

svn_error_t *
ConflictResolverCallback::resolveConflict(svn_wc_conflict_result_t *result,
                                          const svn_wc_conflict_description_t *
                                          desc,
                                          void *baton,
                                          apr_pool_t *pool)
{
  if (baton)
    return ((ConflictResolverCallback *) baton)->resolve(result, desc, pool);
  else
    return SVN_NO_ERROR;
}

svn_error_t *
ConflictResolverCallback::resolve(svn_wc_conflict_result_t *result,
                                  const svn_wc_conflict_description_t *desc,
                                  apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // As Java method IDs will not change during the time this library
  // is loaded, they can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      // Initialize the callback method ID.
      jclass clazz = env->FindClass(JAVA_PACKAGE "/ConflictResolverCallback");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "resolve",
                             "(L" JAVA_PACKAGE "/ConflictDescriptor;)I");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz);
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;
    }

  // Create an instance of the conflict descriptor.
  static jmethodID ctor = 0;
  jclass clazz = env->FindClass(JAVA_PACKAGE "/ConflictDescriptor");
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  if (ctor == 0)
    {
      ctor = env->GetMethodID(clazz, "<init>", "(Ljava/lang/String;IZ"
                              "Ljava/lang/String;IILjava/lang/String;"
                              "Ljava/lang/String;Ljava/lang/String;"
                              "Ljava/lang/String;)V");
      if (JNIUtil::isJavaExceptionThrown() || ctor == 0)
        return SVN_NO_ERROR;
    }

  jstring jpath = JNIUtil::makeJString(desc->path);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  jstring jmimeType = JNIUtil::makeJString(desc->mime_type);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  jstring jbasePath = JNIUtil::makeJString(desc->base_file);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  jstring jreposPath = JNIUtil::makeJString(desc->their_file);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  jstring juserPath = JNIUtil::makeJString(desc->my_file);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  jstring jmergedPath = JNIUtil::makeJString(desc->merged_file);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // Instantiate the conflict descriptor.
  jobject jdesc = env->NewObject(clazz, ctor, jpath,
                                 EnumMapper::mapNodeKind(desc->node_kind),
                                 (jboolean) desc->is_binary, jmimeType,
                                 EnumMapper::mapConflictAction(desc->action),
                                 EnumMapper::mapConflictReason(desc->reason),
                                 jbasePath, jreposPath, juserPath,
                                 jmergedPath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  env->DeleteLocalRef(clazz);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // Invoke the Java conflict resolver callback method using the descriptor.
  jint jresult = env->CallIntMethod(m_conflictResolver, mid, jdesc);
  if (JNIUtil::isJavaExceptionThrown())
    {
      // If an exception is thrown by our conflict resolver, remove it
      // from the JNI env, and convert it into a Subversion error.
      const char *msg = JNIUtil::thrownExceptionToCString();
      return svn_error_create(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE, NULL, msg);
    }
  *result = javaResultToC(jresult);

  env->DeleteLocalRef(jpath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  env->DeleteLocalRef(jmimeType);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  env->DeleteLocalRef(jbasePath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  env->DeleteLocalRef(jreposPath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  env->DeleteLocalRef(juserPath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;
  env->DeleteLocalRef(jmergedPath);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  env->DeleteLocalRef(jdesc);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  return SVN_NO_ERROR;
}

svn_wc_conflict_result_t ConflictResolverCallback::javaResultToC(jint result)
{
  switch (result)
    {
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_conflicted:
    default:
      return svn_wc_conflict_result_conflicted;
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_resolved:
      return svn_wc_conflict_result_resolved;
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_choose_base:
      return svn_wc_conflict_result_choose_base;
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_choose_theirs:
      return svn_wc_conflict_result_choose_theirs;
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_choose_mine:
      return svn_wc_conflict_result_choose_mine;
    case org_tigris_subversion_javahl_ConflictResolverCallback_Result_choose_merged:
      return svn_wc_conflict_result_choose_merged;
    }
}
