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
class FeedsFileFetcherResult extends FeedsFetcherResult {
  /**
   * Constructor.
   */
  public function __construct($file_path) {
    parent::__construct('');
    $this->file_path = $file_path;
  }

  /**
   * Overrides parent::getRaw();
   */
  public function getRaw() {
    return file_get_contents($this->file_path);
  }

  /**
   * Overrides parent::getFilePath().
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
    return new FeedsFileFetcherResult($source_config['source']);
  }

  /**
   * Source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['fid'] = array(
      '#type' => 'value',
      '#value' => empty($source_config['fid']) ? 0 : $source_config['fid'],
    );
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
      '#file_info' => empty($source_config['fid']) ? NULL : file_load($source_config['fid']),
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
    // @todo: Track usage of file, remove file when removing source.
    if ($file = file_save_upload('feeds', array('file_validate_extensions' => array(0 => $this->config['allowed_extensions'])), $feed_dir)) {
      $values['source'] = $file->uri;
      $values['file'] = $file;
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
   * Override parent::sourceSave().
   */
  public function sourceSave(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);

    // If a new file is present, delete the old one and replace it with the new
    // one.
    if (isset($source_config['file'])) {
      $file = $source_config['file'];
      if (isset($source_config['fid'])) {
        $this->deleteFile($source_config['fid'], $source->feed_nid);
      }
      $file->status = FILE_STATUS_PERMANENT;
      file_save($file);
      file_usage_add($file, 'feeds', get_class($this), $source->feed_nid);

      $source_config['fid'] = $file->fid;
      unset($source_config['file']);
      $source->setConfigFor($this, $source_config);
    }
  }

  /**
   * Override parent::sourceDelete().
   */
  public function sourceDelete(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    if (isset($source_config['fid'])) {
      $this->deleteFile($source_config['fid'], $source->feed_nid);
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

  /**
   * Helper. Deletes a file.
   */
  protected function deleteFile($fid, $feed_nid) {
    if ($file = file_load($fid)) {
      file_usage_delete($file, 'feeds', get_class($this), $feed_nid);
      file_delete($file);
    }
  }
}
