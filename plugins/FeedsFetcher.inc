<?php
// $Id$

/**
 * Defines the object a Fetcher returns on fetch().
 */
class FeedsFetcherResult extends FeedsResult {
  // Define valid types.
  // @todo: does text/filepath make sense?
  // @todo: If convenient, we could expand on this concept and build content type negotiation
  // between Fetchers and Parsers.
  protected $validTypes = array('text/filepath', 'text/xml');
}

/**
 * Abstract class, defines shared functionality between fetchers.
 *
 * Implements FeedsSourceInfoInterface to expose source forms to Feeds.
 */
abstract class FeedsFetcher extends FeedsConfigurable implements FeedsSourceInterface {

  // Holds source information.
  private $source_config;

  /**
   * Constructor.
   *
   * Initialize class variables.
   */
  protected function __construct($id) {
    parent::__construct($id);
    $this->source_config = $this->getSourceDefaults();
  }

  /**
   * Implementation of FeedsSourceInterface::getSourceConfig().
   */
  public function getSourceConfig() {
    return $this->source_config;
  }

  /**
   * Implementation of FeedsSourceInterface::setSourceConfig().
   */
  public function setSourceConfig($config) {
    dsm($config);
    $this->source_config = $config;
  }

  /**
   * Implementation of FeedsSourceInterface::getSourceDefaults().
   */
  public function getSourceDefaults() {
    $values = array_keys($this->sourceForm());
    foreach ($values as $k => $v) {
      $values[$k] = '';
    }
    return $values;
  }

  /**
   * Callback methods, exposes source form.
   *
   * @todo: move HTTP fetcher specific code down into HTTP fetcher.
   */
  public function sourceForm() {
    $form = array();
    $form['url'] = array(
      '#type' => 'textfield',
      '#title' => t('URL'),
      '#description' => t('Enter the URL for this feed.'),
      '#default_value' => $this->source_config['url'],
    );
    return $form;
  }

  /**
   * Validation handler for sourceForm.
   */
  public function sourceFormValidate($form, &$values) {}

  /**
   * Submit handler for sourceForm.
   */
  public function sourceFormSubmit($form, &$values) {
    dsm($values);
    $this->source_config['url'] = $values['url'];
  }

  /**
   * Fetch content from a source and return it.
   *
   * Stub method. Every class that extends FeedsFetcher must implement this method.
   *
   * @param $source
   *   Source value as entered by user through sourceForm().
   *
   * @todo: Define format of return value?
   */
  public abstract function fetch(FeedsSource $source);

  /**
   * Save changes to the configuration of this object.
   * Delegate saving to parent Feed which will collect
   * information from this object and store it.
   */
  public function save() {
    $feed = feeds_feed_instance($this->id);
    $feed->save();
  }
}
