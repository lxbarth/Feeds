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
   * Callback methods, exposes source form.
   *
   * @todo: move HTTP fetcher specific code down into HTTP fetcher.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['url'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter the URL for this feed.'),
      '#default_value' => isset($source_config['url']) ? $source_config['url'] : '',
    );
    return $form;
  }

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

