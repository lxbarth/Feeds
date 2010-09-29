<?php
// $Id: file.inc,v 1.1.2.1 2010/09/29 23:56:10 alexb Exp $

/**
 * @file
 * On behalf implementation of Feeds mapping API for file.module and
 * image.module.
 *
 * Does actually not include mappers for field types defined in fields module
 * (because there aren't any) but mappers for all fields that contain their
 * value simply in $entity->fieldname['und'][$i]['value'].
 */

/**
 * Implements hook_feeds_processor_targets_alter().
 *
 * @see FeedsNodeProcessor::getMappingTargets().
 */
function file_feeds_processor_targets_alter(&$targets, $entity_type, $bundle_name) {
  foreach (field_info_instances($entity_type, $bundle_name) as $name => $instance) {
    $info = field_info_field($name);

    if (in_array($info['type'], array('file', 'image'))) {
      $targets[$name] = array(
        'name' => $instance['label'],
        'callback' => 'file_feeds_set_target',
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
function file_feeds_set_target($entity, $target, $value) {
  if (empty($value)) {
    return;
  }
  module_load_include('inc', 'file');

  // Make sure $value is an array of objects of type FeedsEnclosure.
  if (!is_array($value)) {
    $value = array($value);
  }
  foreach ($value as $k => $v) {
    if (!($v instanceof FeedsEnclosure)) {
      if (is_string($v)) {
        $value[$k] = new FeedsEnclosure($v, 'application/octet-stream');
      }
      else {
        unset($value[$k]);
      }
    }
  }
  if (empty($value)) {
    return;
  }

  // Determine file destination.
  // @todo This needs review and debugging.
  $info = field_info_field($target);
  $bundle_name = $entity->entity_type == 'node' ? $entity->type : $entity->entity_type;
  $instance_info = field_info_instance($entity->entity_type, $target, $bundle_name);
  $account = !empty($entity->uid) ? user_load($entity->uid) : NULL;
  $destination = file_field_widget_uri($info, $instance_info, $account);

  // Populate entity.
  $i = 0;
  $field = isset($entity->$target) ? $entity->$target : array();
  foreach ($value as $v) {
    if ($file = $v->getFile($destination)) {
      $field['und'][$i] = (array)$file;
      $field['und'][$i]['display'] = 1; // @todo: Figure out how to properly populate this field.
      if ($info['cardinality'] == 1) {
        break;
      }
      $i++;
    }
  }
  $entity->{$target} = $field;
}
