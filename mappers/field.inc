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
    $allowed_types = array(
      'number_integer',
      'number_decimal',
      'number_float',
      'text',
      'text_long',
      'text_with_summary',
    );
    if (in_array($info['type'], $allowed_types)) {
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
  $info = field_info_field($target);

  // Handle non-multiple value fields.
  if (!is_array($value)) {
    $value = array($value);
  }

  // Iterate over all values.
  $i = 0;
  $field = isset($node->$target) ? $node->$target : array();
  foreach ($value as $v) {
    if (!is_array($v) && !is_object($v)) {
      $field['und'][$i]['value'] = $v;
    }
    if ($info['cardinality'] == 1) {
      break;
    }
    $i++;
  }
  $node->$target = $field;
}
