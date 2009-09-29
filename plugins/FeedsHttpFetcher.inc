<?php
// $Id: FeedsHttpFetcher.inc,v 1.1 2009/09/16 17:58:13 alexb Exp $
/**
 * @file
 * Home of the FeedsHttpFetcher.
 *
 */

/**
 * Fetches data via HTTP.
 */
class FeedsHttpFetcher extends FeedsFetcher {

  /**
   * Fetch a resource via http.
   * 
   * @todo: Use Curl, fall back to drupal_http_request.
   * @todo: Add caching.
   *
   * @param $resource
   *   A resource description of type FeedsResource.
   * 
   * @return 
   *   A string from the requested location if successful, or FALSE if not.
   */
  public function fetch($resource) {
    if ($resource->type == 'http') {
      $result = drupal_http_request($resource->identifier);
      $count = 0;
      while ($result->redirect_url) {
        $result = drupal_http_request($result->redirect_url);
        $count++;
        if ($count > 100) {
          return FALSE;
        }
      }
      if (!$result->error) {
        return $result->response;
      }
      return FALSE;
    }
  }
}