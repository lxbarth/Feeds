<?php
// $Id: FeedsFileFetcher.inc,v 1.21 2010/09/20 18:30:17 alexb Exp $

/**
 * @file
 * Home of the FeedsFileFetcher and related classes.
 */

/**
 * Definition of the import batch object created on the fetching stage by
 * FeedsFileFetcher.
 */
class FeedsFileBatch extends FeedsImportBatch {
  /**
   * Constructor.
   */
  public function __construct($file_path, $feed_nid = 0) {
    parent::__construct('', $feed_nid);
    $this->file_path = $file_path;
  }

  /**
   * Implements FeedsImportBatch::getRaw();
   */
  public function getRaw() {
    return file_get_contents($this->file_path);
  }

  /**
   * Implements FeedsImportBatch::getFilePath().
   */
  public function getFilePath() {
    if (!file_exists($this->file_path)) {
      throw new Exception(t('File @filepath is not accessible.', array('@filepath' => $this->file_path)));
    }
    return $this->file_path;
  }
}

/**
 * Fetches data via HTTP.
 */
class FeedsFileFetcher extends FeedsFetcher {

  /**
   * Implements FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    return new FeedsFileBatch($source_config['source'], $source->feed_nid);
  }

  /**
   * Source form.
   */
  public function sourceForm($source_config) {
    $form = $info = array();
    if (!empty($source_config['source']) && file_exists($source_config['source'])) {
      $wrapper = file_stream_wrapper_get_instance_by_uri($source_config['source']);
      $path = $wrapper->getExternalUrl();
      $info = array(
        'path' => $path,
        'size' => filesize($source_config['source']),
      );
      if (module_exists('mimedetect')) {
        $info['mime'] = mimedetect_mime($source_config['source']);
      }
    }
    $form['source'] = array(
      '#type' => empty($this->config['direct']) ? 'value' : 'textfield',
      '#title' => t('File'),
      '#description' => t('Specify a file in the site\'s file system path or upload a file below.'),
      '#default_value' => empty($source_config['source']) ? '' : $source_config['source'],
    );
    $form['upload'] = array(
      '#type' => 'file',
      '#title' => empty($this->config['direct']) ? t('File') : NULL,
      '#description' => empty($source_config['source']) ? t('Select the file to be imported from your local system.') : t('Select a different file to be imported from your local system.'),
      '#theme' => 'feeds_upload',
      '#file_info' => $info,
      '#size' => 10,
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    $feed_dir = 'public://feeds';
    file_prepare_directory($feed_dir, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);

    // If there is a file uploaded, save it, otherwise validate input on
    // file.
    if ($file = file_save_upload('feeds', array('file_validate_extensions' => array(0 => $this->config['allowed_extensions'])), $feed_dir)) {
      $values['source'] = $file->uri;
    }
    elseif (empty($values['source'])) {
      form_set_error('feeds][source', t('Upload a file first.'));
    }
    // If a file has not been uploaded and $values['source'] is not empty, make
    // sure that this file is within Drupal's files directory as otherwise
    // potentially any file that the web server has access could be exposed.
    elseif (strpos($values['source'], 'public://') !== 0) {
      form_set_error('feeds][source', t('File needs to reside within the site\'s file directory, its path needs to start with public://.'));
    }
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'allowed_extensions' => 'txt csv xml',
      'direct' => FALSE,
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['allowed_extensions'] = array(
      '#type' =>'textfield',
      '#title' => t('Allowed file extensions'),
      '#description' => t('Allowed file extensions for upload.'),
      '#default_value' => $this->config['allowed_extensions'],
    );
    $form['direct'] = array(
      '#type' =>'checkbox',
      '#title' => t('Supply path to file directly'),
      '#description' => t('For experts. If checked users can specify a path to a file when importing rather than uploading a file. This is useful when files to be imported are already present on server.'),
      '#default_value' => $this->config['direct'],
    );
    return $form;
  }
}
