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
  public function fetch(FeedsSource $source) {
    $url = $source->config['url'];
    feeds_include('http_request', 'libraries');
    if ($this->config['auto_detect_feeds']) {
      return new FeedsFetcherResult(http_request_get_common_syndication($url), 'text/xml');
    }
    else {
      return new FeedsFetcherResult(http_request_get($url), 'text/xml');
    }
  }

  /**
   * Override parent::getDefaultConfig().
   */
  public function getDefaultConfig() {
    return array('auto_detect_feeds' => FALSE);
  }

  /**
   * Configuration form.
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['auto_detect_feeds'] = array(
      '#type' => 'checkbox',
      '#title' => t('Auto detect feeds'),
      '#description' => t('Auto detect RSS or Atom feeds in the downloaded document and use them instead of the given URL.'),
      '#default_value' => $this->config['auto_detect_feeds'],
      );
    return $form;
  }
}

