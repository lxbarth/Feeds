<?php
// $Id$

/**
 * @file
 * Home of FeedsSource and related interfaces and exceptions.
 */

/**
 * Describes a class that defines default values and form descriptions for
 * a FeedSource.
 *
 * @todo: comment.
 */
interface FeedsSourceInterface {
  /**
   * Crook: for ease of use, we implement FeedsSourceInterface for every plugin,
   * but then we need to have a handle which plugin actually implements a source.
   *
   * @see FeedsPlugin class.
   *
   * @return
   *   TRUE if a plugin handles source specific configuration, FALSE otherwise.
   */
  public function hasSourceConfig();
  public function setSourceConfig($source_config);
  public function getSourceDefaults();
  public function sourceForm();
  public function sourceFormValidate($form, &$values);
  public function sourceFormSubmit($form, &$values);
}

/**
 * This exception gets thrown when no source is available
 * for a given feed_nid.
 *
 * @todo: don't use internal source loading + exception, but load source
 * object and pass it into Feed object.
 */
class FeedsNoSourceException extends Exception {}

/**
 * This class encapsulates a source of a feed. While a Feed object contains
 * a feed import configuration, a FeedsSource object is what passes through
 * such an import configuration, holds information about the feed's source
 * (e. g. the URL) and provides all the information for proper fetching,
 * parsing and processing of a feed source.
 *
 * As with Feed, the idea with FeedsSource is that it can be used without
 * actually saving the object to the database.
 *
 * @todo: Call this FeedsSourceInfo ?
 */
class FeedsSource extends FeedsConfigurable {

  // Contains the node id of the feed this source info object is attached to.
  // If not attached to any node, contains 0 - i. e. if used on a
  // standalone import form within Feeds or by other API users.
  protected $feed_nid;

  // The Feed object that this source is expected to be used with.
  protected $feed;

  /**
   * Instantiate a unique object per class/id/feed_nid. Don't use
   * directly, use feeds_source_instance() instead.
   *
   * @todo: I thought overriding static functions would not work in PHP.
   */
  public static function instance($class, $feed, $feed_nid = 0) {
    // This is useful at least as long as we're developing.
    if (empty($feed)) {
      throw new Exception('Empty configuration identifier.');
    }
    static $instances = array();
    if (!isset($instances[$class][$id][$feed_nid])) {
      $instances[$class][$id][$feed_nid] = new $class($feed, $feed_nid);
    }
    return $instances[$class][$id][$feed_nid];
  }

  /**
   * Constructor.
   */
  protected function __construct($feed, $feed_nid) {
    $this->feed_nid = $feed_nid;
    $this->feed = $feed;
    parent::__construct($feed->id);
    $this->load();
  }

  /**
   * Save configuration.
   */
  public function save() {
    $object = array(
      'id' => $this->id,
      'feed_nid' => $this->feed_nid,
      'config' => $this->getConfig(),
    );
    // Make sure a source record is present at all time, try to update first, then insert.
    drupal_write_record('feeds_source', $object, array('id', 'feed_nid'));
    if (!db_affected_rows()) {
      drupal_write_record('feeds_source', $object);
    }
  }

  /**
   * Load configuration and unpack.
   */
  public function load() {
    if ($config = db_result(db_query('SELECT config FROM {feeds_source} WHERE id = "%s" AND feed_nid = %d', $this->id, $this->feed_nid))) {
      // While FeedsSource cannot be exported, we still use CTool's export.inc export definitions.
      // @todo: patch CTools to move constants from export.inc to ctools.module.
      ctools_include('export');
      $this->export_type = EXPORT_IN_DATABASE;
      $this->config = unserialize($config);

      // Invoke setter to set all members.
      $this->setConfig($this->config);
    }
  }

  /**
   * Delete configuration. Removes configuration information
   * from database, does not delete configuration itself.
   */
  public function delete() {
    db_query('DELETE FROM {feeds_source} WHERE id = "%s" AND feed_nid = %d', $this->id, $this->feed_nid);
  }

  /**
   * Set configuration.
   *
   * @param $config
   *   Array containing configuration information. Will be filtered by the keys returned by
   *   getDefaultConfig().
   */
  public function setConfig($config) {
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if (isset($config[$type]) && $this->feed->$type->hasSourceConfig()) {
        $this->feed->$type->setSourceConfig($config[$type]);
      }
    }
  }

  /**
   * Get configuration, iterate over members and retrieve their configuration.
   *
   * Internally, use $this->getConfig() to pull the configuration status of member
   * objects on $this->feed. External usage of $object->config will be funnelled through
   * this method by parent::__get().
   */
  public function getConfig() {
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if ($this->feed->$type->hasSourceConfig()) {
        // @todo: invoke getSourceDefaults() if getSourceConfig() does not yield results?
        // @todo: key in class name, otherwise inconsistent values may occur
        $this->config[$type] = $this->feed->$type->getSourceConfig();
      }
    }
    return $this->config;
  }

  /**
   * Return defaults for feed configuration.
   */
  public function getDefaultConfig() {
    // Collect information from plugins.
    $defaults = array();
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if ($this->feed->$type->hasSourceConfig()) {
        $defaults[$type] = $this->feed->$type->getSourceDefaults();
      }
    }
    return $defaults;
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    // Collect information from plugins.
    $form = array();
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if ($this->feed->$type->hasSourceConfig()) {
        $form[$type] = $this->feed->$type->sourceForm();
        $form[$type]['#tree'] = TRUE;
      }
    }
    return $form;
  }

  /**
   * Override parent::configFormValidate().
   *
   * @todo: make configFormValidate($form, &$values) in parent, too.
   * @todo: push declaration into FeedsConfigurableInterface
   * @todo: make FeedsConfigurable an abstract class by declaring stubs of these functions.
   */
  public function configFormValidate($form, &$values) {
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if (isset($values[$type]) && $this->feed->$type->hasSourceConfig()) {
        $this->feed->$type->sourceFormValidate($form, $values[$type]);
      }
    }
  }

  /**
   * Override parent::configFormSubmit().
   */
  public function configFormSubmit($form, &$values) {
    foreach (array('fetcher', 'parser', 'processor') as $type) {
      if (isset($values[$type]) && $this->feed->$type->hasSourceConfig()) {
        $this->feed->$type->sourceFormSubmit($form, $values[$type]);
      }
    }
  }
}
