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
 * @todo: Call IFeedsSourceClient?
 * @todo: comment.
 */
interface FeedsSourceInterface {
  /**
   * Crook: for ease of use, we implement FeedsSourceInterface for every plugin,
   * but then we need to have a handle which plugin actually implements a
   * source.
   *
   * @see FeedsPlugin class.
   *
   * @return
   *   TRUE if a plugin handles source specific configuration, FALSE otherwise.
   */
  public function hasSourceConfig();
  public function sourceDefaults();
  public function sourceForm($source_config);
  public function sourceFormValidate(&$values);
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
 * @todo: Rename to FeedsSourceInfo.
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
   * directly, use feeds_source() instead.
   *
   * @todo: Does overriding static functions work in PHP?
   */
  public static function instance($feed, $feed_nid = 0) {
    // This is useful at least as long as we're developing.
    if (empty($feed)) {
      throw new Exception('Empty feed.');
    }

    // Let others override which class to instantiate.
    $class = variable_get('feeds_source_class', 'FeedsSource');

    static $instances = array();
    if (!isset($instances[$class][$feed->id][$feed_nid])) {
      $instances[$class][$feed->id][$feed_nid] = new $class($feed, $feed_nid);
    }
    return $instances[$class][$feed->id][$feed_nid];
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
    // Make sure a source record is present at all time, try to update first,
    // then insert.
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
      // While FeedsSource cannot be exported, we still use CTool's export.inc
      // export definitions.
      // @todo: patch CTools to move constants from export.inc to ctools.module.
      ctools_include('export');
      $this->export_type = EXPORT_IN_DATABASE;
      $this->config = unserialize($config);
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
   * Convenience function. Returns the configuration for a specific class.
   *
   * @param FeedsSourceInterface $client
   *   An object that is an implementer of FeedsSourceInterface.
   *
   * @return
   *   An array stored for $client.
   */
  public function getConfigFor(FeedsSourceInterface $client) {
    return $this->config[get_class($client)];
  }

  /**
   * Return defaults for feed configuration.
   */
  public function configDefaults() {
    // Collect information from plugins.
    $defaults = array();
    foreach ($this->feed->plugin_types as $type) {
      if ($this->feed->$type->hasSourceConfig()) {
        $defaults[get_class($this->feed->$type)] = $this->feed->$type->sourceDefaults();
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
    foreach ($this->feed->plugin_types as $type) {
      if ($this->feed->$type->hasSourceConfig()) {
        $class = get_class($this->feed->$type);
        $form[$class] = $this->feed->$type->sourceForm($this->config[$class]);
        $form[$class]['#tree'] = TRUE;
      }
    }
    return $form;
  }

  /**
   * Override parent::configFormValidate().
   */
  public function configFormValidate(&$values) {
    foreach ($this->feed->plugin_types as $type) {
      $class = get_class($this->feed->$type);
      if (isset($values[$class]) && $this->feed->$type->hasSourceConfig()) {
        $this->feed->$type->sourceFormValidate($values[$class]);
      }
    }
  }
}
