<?php
// $Id: feeds.api.php,v 1.17.2.1 2010/09/29 23:56:09 alexb Exp $

/**
 * @mainpage
 *
 * @verbinclude README.txt
 */

/**
 * Feeds offers a CTools based plugin API. Fetchers, parsers and processors are
 * declared to Feeds as plugins.
 *
 * @see feeds_feeds_plugins()
 * @see FeedsFetcher
 * @see FeedsParser
 * @see FeedsProcessor
 *
 * @defgroup pluginapi Plugin API
 * @{
 */

/**
 * Example of a CTools plugin hook that needs to be implemented to make
 * hook_feeds_plugins() discoverable by CTools and Feeds. The hook specifies
 * that the hook_feeds_plugins() returns Feeds Plugin API version 1 style
 * plugins.
 */
function hook_ctools_plugin_api($owner, $api) {
  if ($owner == 'feeds' && $api == 'plugins') {
    return array('version' => 1);
  }
}

/**
 * A hook_feeds_plugins() declares available Fetcher, Parser or Processor
 * plugins to Feeds. For an example look at feeds_feeds_plugin(). For exposing
 * this hook hook_ctools_plugin_api() MUST be implemented, too.
 *
 * @see feeds_feeds_plugin()
 */
function hook_feeds_plugins() {
  $info = array();
  $info['MyFetcher'] = array(
    'name' => 'My Fetcher',
    'description' => 'Fetches my stuff.',
    'help' => 'More verbose description here. Will be displayed on fetcher selection menu.',
    'handler' => array(
      'parent' => 'FeedsFetcher',
      'class' => 'MyFetcher',
      'file' => 'MyFetcher.inc',
      'path' => drupal_get_path('module', 'my_module'), // Feeds will look for MyFetcher.inc in the my_module directory.
    ),
  );
  $info['MyParser'] = array(
    'name' => 'ODK parser',
    'description' => 'Parse my stuff.',
    'help' => 'More verbose description here. Will be displayed on parser selection menu.',
    'handler' => array(
      'parent' => 'FeedsParser', // Being directly or indirectly an extension of FeedsParser makes a plugin a parser plugin.
      'class' => 'MyParser',
      'file' => 'MyParser.inc',
      'path' => drupal_get_path('module', 'my_module'),
    ),
  );
  $info['MyProcessor'] = array(
    'name' => 'ODK parser',
    'description' => 'Process my stuff.',
    'help' => 'More verbose description here. Will be displayed on processor selection menu.',
    'handler' => array(
      'parent' => 'FeedsProcessor',
      'class' => 'MyProcessor',
      'file' => 'MyProcessor.inc',
      'path' => drupal_get_path('module', 'my_module'),
    ),
  );
  return $info;
}

/**
 * @}
 */

/**
 * @defgroup import Import and clear hooks
 * @{
 */

/**
 * Invoked after a feed source has been parsed, before it will be processed.
 *
 * @param $source
 *  FeedsSource object that describes the source that has been imported.
 * @param $result
 *   FeedsParserResult object that has been parsed from the source.
 */
function hook_feeds_after_parse(FeedsSource $source, FeedsParserResult $result) {
  // For example, set title of imported content:
  $result->setTitle('Import number ' . my_module_import_id());
}

/**
 * Invoked after a feed source has been imported.
 *
 * @param $source
 *  FeedsSource object that describes the source that has been imported.
 */
function hook_feeds_after_import(FeedsSource $source) {
  // See geotaxonomy module's implementation for an example.
}

/**
 * Invoked after a feed source has been cleared of its items.
 *
 * @param $source
 *  FeedsSource object that describes the source that has been cleared.
 */
function hook_feeds_after_clear(FeedsSource $source) {
}

/**
 * @}
 */

/**
 * @defgroup mappingapi Mapping API
 * @{
 */

/**
 * Alter mapping sources.
 *
 * Use this hook to add additional mapping sources for any parser. Allows for
 * registering a callback to be invoked at mapping time.
 *
 * @see my_source_get_source().
 * @see locale_feeds_parser_sources_alter().
 */
function hook_feeds_parser_sources_alter(&$sources, $content_type) {
  $sources['my_source'] = array(
    'name' => t('Images in description element'),
    'description' => t('Images occuring in the description element of a feed item.'),
    'callback' => 'my_source_get_source',
  );
}

/**
 * Example callback specified in hook_feeds_parser_sources_alter().
 *
 * To be invoked on mapping time.
 *
 * @param $source
 *   The FeedsSource object being imported.
 * @param $result
 *   The FeedsParserResult object being mapped from.
 * @param $key
 *   The key specified in the $sources array in
 *   hook_feeds_parser_sources_alter().
 *
 * @return
 *   The value to be extracted from the source.
 *
 * @see hook_feeds_parser_sources_alter().
 * @see locale_feeds_get_source().
 */
function my_source_get_source(FeedsSource $source, FeedsParserResult $result, $key) {
  $item = $result->currentItem();
  return my_source_parse_images($item['description']);
}

/**
 * Alter mapping targets for entities. Use this hook to add additional target
 * options to the mapping form of Node processors.
 *
 * If the key in $targets[] does not correspond to the actual key on the node
 * object ($node->key), real_target MUST be specified. See mappers/link.inc
 *
 * For an example implementation, see mappers/content.inc
 *
 * @param &$targets
 *   Array containing the targets to be offered to the user. Add to this array
 *   to expose additional options. Remove from this array to suppress options.
 *   Remove with caution.
 * @param $entity_type
 *   The entity type of the target, for instance a 'node' entity.
 * @param $bundle_name
 *   The bundle name for which to alter targets.
 */
function hook_feeds_processor_targets_alter(&$targets, $entity_type, $bundle_name) {
  if ($entity_type == 'node') {
    $targets['my_node_field'] = array(
      'name' => t('My custom node field'),
      'description' => t('Description of what my custom node field does.'),
      'callback' => 'my_module_set_target',
    );
    $targets['my_node_field2'] = array(
      'name' => t('My Second custom node field'),
      'description' => t('Description of what my second custom node field does.'),
      'callback' => 'my_module_set_target2',
      'real_target' => 'my_node_field_two', // Specify real target field on node.
    );
  }
}

/**
 * Example callback specified in hook_feeds_processor_targets_alter().
 *
 * @param $entity
 *   An entity object, for instance a node object.
 * @param $target
 *   A string identifying the target on the node.
 * @param $value
 *   The value to populate the target with.
 *
 */
function my_module_set_target($entity, $target, $value) {
  $entity->$target['und'][0]['value'] = $value;
}

/**
 * @}
 */
