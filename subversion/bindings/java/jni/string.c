/*
 * utility functions to deal with strings:
 * - java strings (java.lang.String)
 * - svn strings (svn_string_t)
 * - c strings (char *)
 *
 * ====================================================================
 * Copyright (c) 2000-2001 CollabNet.  All rights reserved.
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
 */

/*** Includes ***/
#include <jni.h>
#include <svn_string.h>
#include "misc.h"
#include "global.h"

/*** Code ***/
svn_string_t *
string__j_to_svn_string(JNIEnv *env, 
                        jstring jstr, 
                        jboolean *hasException,
                        apr_pool_t *pool)
{
  svn_string_t *result = NULL;
  jboolean _hasException = JNI_FALSE;

#ifdef SVN_JNI__VERBOSE
  fprintf(stderr, "string__j_to_svn_string\n");
#endif
  
  /* make sure there is enough memory left for 
   * the operation, also push the stack frame
   * we will need 2 local references:
   * - 
   */
  if( (*env)->PushLocalFrame(env, 2) >= 0)
    {
      jbyteArray bytes = NULL;

      /* check for exception */
      _hasException = (*env)->ExceptionCheck(env);

      if( !_hasException )
	{
	  char *buffer;
	  jsize len = (*env)->GetStringUTFLength(env, jstr);
	  buffer = (char *)malloc(len + 1);

	  /* did the memory allocation succeed? 
	   * otherwise throw an exception */
	  if( buffer == NULL )
	    {
	      throw_exception_by_name(env, 
                                      "java/lang/OutOfMemoryError", 
                                      NULL);
	      _hasException = JNI_TRUE;
	    }
	  else
	    {
	      (*env)->GetStringUTFRegion(env, jstr, 0, len, buffer);
	      buffer[len] = 0;

	      if( (*env)->ExceptionCheck(env) )
		{
		  _hasException = JNI_TRUE;
		}

	      if( !_hasException )
		{
		  result = svn_string_create(buffer, pool);
		}

	      /* ...and release the buffer */
	      free(buffer);
	    }
	} /* if( !_hasException ) */

      (*env)->PopLocalFrame(env, NULL);

    }

  /* return wether an exception has occured */
  if( hasException != NULL )
    {
      (*hasException) = _hasException;
    }

  return result;
}

svn_stringbuf_t *
string__c_to_stringbuf(JNIEnv *env,
                       jstring jstr,
                       jboolean *hasException,
                       apr_pool_t *pool)
{
  svn_stringbuf_t *result = NULL;
  svn_string_t *string = NULL;
  jboolean _hasException = JNI_FALSE;

  string = string__j_to_svn_string(env, jstr, &_hasException, pool);

  if( (!_hasException) && (string != NULL ) )
    {
      result = svn_stringbuf_create_from_string(string, pool);

      // did the call succed? otherwise exception
      if( result == NULL )
        {
          misc__throw_exception_by_name(env, 
                                        SVN_JNI_SUBVERSION_EXCEPTION,
                                        "svn_stringbuf_create_from_string" \
                                        " failed");
          _hasException = JNI_TRUE;
        }
    }

  if( hasException != NULL )
    {
      *hasException = _hasException;
    }

  return result;
}

jstring
string__c_to_j(JNIEnv *env, 
               char *string, 
               jboolean *hasException)
{
  jboolean _hasException = JNI_FALSE;
  jstring result = NULL;

  result = (*env)->NewStringUTF(env, string);

  if( (*env)->ExceptionCheck(env) )
    {
      _hasException = JNI_TRUE;
    }

  if( hasException != NULL )
    {
      (*hasException) = _hasException;
    }

  return result;
}

jstring
string__svn_string_to_j(JNIEnv *env, 
                        svn_string_t *string, 
                        jboolean *hasException)
{
  return string__c_to_j(env, (char*)string->data, 
                        hasException);
}

jstring
string__svn_stringbuf_to_j(JNIEnv *env,
                           svn_stringbuf_t *stringbuf,
                           jboolean *hasException)
{
  return string__c_to_j(env, (char*)stringbuf->data,
                        hasException);
}

/* 
 * local variables:
 * eval: (load-file "../../../svn-dev.el")
 * end: 
 */





