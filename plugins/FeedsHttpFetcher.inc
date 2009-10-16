<?php
// $Id: FeedsHttpFetcher.inc,v 1.1 2009/09/16 17:58:13 alexb Exp $

/**
 * @file
 * Home of the FeedsHttpFetcher.
 * @todo: Use all caps for HTTP.
 */

/**
 * Fetches data via HTTP.
 */
class FeedsHttpFetcher extends FeedsFetcher {

  /**
   * Fetch a resource via http.
   *
   * @param $resource
   *   A resource description of type FeedsResource.
   *
   * @return
   *   A string from the requested location if successful, or FALSE if not.
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $url = $source_config['url'];
    feeds_include_library('http_request.inc', 'http_request');
    if ($this->config['auto_detect_feeds']) {
      $result = http_request_get_common_syndication($url);
    }
    else {
      $result = http_request_get($url);
    }
    return new FeedsFetcherResult($result->data, 'text/xml');
  }

  /**
   * Clear caches.
   */
  public function expire(FeedsSource $source, $time) {
    $source_config = $source->getConfigFor($this);
    $url = $source_config['url'];
    feeds_include_library('http_request.inc', 'http_request');
    http_request_clear_cache($url);
  }

  /**
   * Expose source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['url'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter a feed URL.'),
      '#default_value' => isset($source_config['url']) ? $source_config['url'] : '',
      '#required' => TRUE,
    );
    return $form;
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array('auto_detect_feeds' => FALSE);
  }

  /**
   * Configuration form.
   */
  public function configForm(&$form_state) {
    $form = array();
    return $form;
    // @todo:
    $form['auto_detect_feeds'] = array(
      '#type' => 'checkbox',
      '#title' => t('Auto detect feeds (not tested)'),
      '#description' => t('Auto detect RSS or Atom feeds in the downloaded document and use them instead of the given URL.'),
      '#default_value' => $this->config['auto_detect_feeds'],
      );
    return $form;
  }
}

