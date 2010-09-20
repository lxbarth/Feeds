<?php
// $Id$

/**
 * @file
 * On behalf implementation of Feeds mapping API for field.module.
 */

/**
 * Implements hook_feeds_node_processor_targets_alter().
 *
 * @see FeedsNodeProcessor::getMappingTargets().
 */
function field_feeds_node_processor_targets_alter(&$targets, $content_type) {
  foreach (field_info_instances('node', $content_type) as $name => $instance) {
    $info = field_info_field($name);
    if (in_array($info['type'], array('text_with_summary'))) {
      $targets[$name] = array(
        'name' => $instance['label'],
        'callback' => 'field_feeds_set_target',
        'description' => t('The @label field of the node.', array('@label' => $instance['label'])),
      );
    }
  }
}

/**
 * Callback for mapping. Here is where the actual mapping happens.
 *
 * When the callback is invoked, $target contains the name of the field the
 * user has decided to map to and $value contains the value of the feed item
 * element the user has picked as a source.
 */
function field_feeds_set_target($node, $target, $value) {

  $field = isset($node->$target) ? $node->$target : array();

  // Handle multiple value fields.
  if (is_array($value)) {
    $i = 0;
    foreach ($value as $v) {
      if (!is_array($v) && !is_object($v)) {
        $field[$i]['value'] = $v;
      }
      $i++;
    }
  }
  else {
    $field[0]['value'] = $value;
  }

  $node->$target = $field;
}
