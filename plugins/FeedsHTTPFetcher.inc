<?php
// $Id: FeedsHTTPFetcher.inc,v 1.13 2010/02/18 20:13:31 alexb Exp $

/**
 * @file
 * Home of the FeedsHTTPFetcher and related classes.
 */

define('FEEDS_HTTP_NO_BASIC_AUTH', 0);
define('FEEDS_HTTP_BASIC_AUTH_PER_SOURCE', 1);
define('FEEDS_HTTP_BASIC_AUTH_PER_IMPORTER', 2);

/**
 * Definition of the import batch object created on the fetching stage by
 * FeedsHTTPFetcher.
 */
class FeedsHTTPBatch extends FeedsImportBatch {
  protected $url;
  protected $file_path;

  /**
   * Constructor.
   */
  public function __construct($url = NULL) {
    $this->url = $url;
    parent::__construct();
  }

  /**
   * Implementation of FeedsImportBatch::getRaw();
   */
  public function getRaw() {
    feeds_include_library('http_request.inc', 'http_request');
    $result = http_request_get($this->url);
    if ($result->code != 200) {
      throw new Exception(t('Download of @url failed with code !code.', array('@url' => $this->url, '!code' => $result->code)));
    }
    return $result->data;
  }

  /**
   * Implementation of FeedsImportBatch::getFilePath().
   */
  public function getFilePath() {
    if (!isset($this->file_path)) {
      $dest = file_destination(file_directory_path() .'/feeds/'. get_class($this) .'_'. md5($this->url) .'_'. time(), FILE_EXISTS_RENAME);
      $this->file_path = file_save_data($this->getRaw(), $dest);
      if($this->file_path === 0) {
        throw new Exception(t('Cannot write content to %dest', array('%dest' => $dest)));
      }
    }
    return $this->file_path;
  }
}

/**
 * Fetches data via HTTP.
 */
class FeedsHTTPFetcher extends FeedsFetcher {

  /**
   * Implementation of FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $url = $source_config['source'];
    $url = $this->basicAuth($url, isset($source_config['basic_auth']) ? $source_config['basic_auth'] : array());
    return new FeedsHTTPBatch($url);
  }

  /**
   * Clear caches.
   */
  public function clear(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $url = $source_config['source'];
    feeds_include_library('http_request.inc', 'http_request');
    http_request_clear_cache($url);
  }

  /**
   * Expose source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter a feed URL.'),
      '#default_value' => isset($source_config['source']) ? $source_config['source'] : '',
      '#maxlength' => NULL,
      '#required' => TRUE,
    );
    if (isset($this->config['basic_auth']) && $this->config['basic_auth'] == FEEDS_HTTP_BASIC_AUTH_PER_SOURCE) {
      $form['basic_auth'] = array(
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => empty($source_config['basic_auth']['basic_auth_user']),
        '#title' => t('Authentication'),
        '#description' => t('Enter user name and password for authentication. Leave empty if no authentication is required.'),
      );
      $form['basic_auth'] += $this->basicAuthForm(isset($source_config['basic_auth']) ? $source_config['basic_auth'] : array());
    }
    return $form;
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'basic_auth' => FEEDS_HTTP_NO_BASIC_AUTH,
      'basic_auth_user' => '',
      'basic_auth_password' => '',
    );
  }

  /**
   * Override parent::configForm();
   */
  public function configForm() {
    $form = array();
    $form['basic_auth'] = array(
      '#type' => 'fieldset',
      '#title' => t('HTTP Basic Authentication'),
    );
    $form['basic_auth']['basic_auth'] = array(
      '#type' => 'radios',
      '#options' => array(
        FEEDS_HTTP_NO_BASIC_AUTH => t('No authentication'),
        FEEDS_HTTP_BASIC_AUTH_PER_SOURCE => t('Specify credentials when creating a feed.'),
        FEEDS_HTTP_BASIC_AUTH_PER_IMPORTER => t('One set of credentials for all feeds.'),
      ),
      '#default_value' => isset($this->config['basic_auth']) ? $this->config['basic_auth'] : FEEDS_HTTP_NO_BASIC_AUTH,
    );
    $form['basic_auth'] += $this->basicAuthForm($this->config);
    return $form;
  }

  /**
   * Validate config form.
   */
  public function configFormValidate(&$values) {
    // Don't accidentally wipe out password.
    if (empty($values['basic_auth_password']) && !empty($this->config['basic_auth_password'])) {
      $values['basic_auth_password'] = $this->config['basic_auth_password'];
    }
    if ($values['basic_auth'] != FEEDS_HTTP_BASIC_AUTH_PER_IMPORTER) {
      $values['basic_auth_user'] = '';
      $values['basic_auth_password'] = '';
    }
  }

  /**
   * Basic auth form.
   */
  protected function basicAuthForm($config) {
    $form = array();
    $form['basic_auth_user'] = array(
      '#type' => 'textfield',
      '#title' => t('Username'),
      '#default_value' => empty($config['basic_auth_user']) ? '' : $config['basic_auth_user'],
    );
    $form['basic_auth_password'] = array(
      '#type' => 'password',
      '#title' => t('Password'),
    );
    return $form;
  }

  /**
   * Encode basic authentication credentials in URL depending on configuration.
   */
  protected function basicAuth($url, $source_config = array()) {
    if (!isset($this->config['basic_auth'])) {
      return $url;
    }
    elseif ($this->config['basic_auth'] == FEEDS_HTTP_BASIC_AUTH_PER_SOURCE) {
      if (!empty($source_config['basic_auth_user']) && !empty($source_config['basic_auth_password'])) {
        return $this->basicAuthEncodeCredentials($url, $source_config['basic_auth_user'], $source_config['basic_auth_password']);
      }
    }
    elseif ($this->config['basic_auth'] == FEEDS_HTTP_BASIC_AUTH_PER_IMPORTER) {
      return $this->basicAuthEncodeCredentials($url, $this->config['basic_auth_user'], $this->config['basic_auth_password']);
    }
    return $url;
  }

  /**
   * Encode basic authentication credentials into URL.
   */
  protected function basicAuthEncodeCredentials($url, $user, $password) {
    $parsed = parse_url($url);
    return str_replace("{$parsed['scheme']}://", "{$parsed['scheme']}://$user:$password@", $url);
  }
}

