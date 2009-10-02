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
  public function sourceForm($nid = NULL) {
    if (!$source = $this->loadSource($nid)) {
      $source = $this->config['source'];
    }
    $form = array();
    $form['feeds_source'] = array(
      '#type' => 'textfield',
      '#title' => t('File'),
      '#description' => t('Specify a file in the site\'s file system path or upload a file below.'),
      '#default_value' => $source,
    );
    $form['feeds_upload'] = array(
      '#type' => 'file',
      '#title' => t('Upload'),
      '#description' => t('Choose a file from your local computer, then click "Import".'),
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate($form, &$values) {
    $feed_dir = file_directory_path() .'/feeds';
    file_check_directory($feed_dir, TRUE);

    // If there is a file uploaded, save it, otherwise validate input on feeds_source.
    if ($file = file_save_upload('feeds', array(), $feed_dir)) {
      file_set_status($file, FILE_STATUS_PERMANENT);
      $values['feeds_source'] = $file->filepath;
    }
    elseif (!file_check_location($values['feeds_source'], file_directory_path())) {
      form_set_error('feeds][feeds_source', t('File needs to point to a file in your Drupal file system path.'));
      $values['feeds_source'] = $this->loadSource();
    }
  }

  /**
   * Fetch a local resource.
   * 
   * @todo: support seemless expanding of archives.
   */
  public function fetch($nid = NULL) {
    // Just return path to file, contents can be read easily with file_get_contents($file_path);
    // @todo: Create a class definition for this.
    return new FeedsFetcherResult($this->loadSource(), 'text/filepath');
  }
}