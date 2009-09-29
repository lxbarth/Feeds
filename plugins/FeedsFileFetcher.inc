<?php
// $Id: FeedsFileFetcher.inc,v 1.1 2009/09/16 22:33:23 alexb Exp $
/**
 * @file
 * Home of the FeedsFileFetcher.
 */

/**
 * Fetches data via HTTP.
 */
class FeedsFileFetcher extends FeedsFetcher {

  /**
   * Source form.
   */
  public function sourceForm(&$form_state) {
    $form = array();
    $form['source'] = array(
      '#type' => 'file',
      '#title' => t('File'),
      '#description' => t('Upload a file.'),
      '#default_value' => $this->config['source'],
    );
    return $form;
  }

  /**
   * Fetch a local resource.
   */
  public function fetch($source) {
    // @todo
  }
}