<?php
// $Id$

/**
 * @file
 * OPML Parser.
 */

/**
 * Parse OPML file.
 *
 * @param $raw
 *   File contents.
 * @return
 *   An array of the parsed OPML file.
 */
function opml_parser_parse($raw) {
  $feeds = $items = array();
  $xml = new SimpleXMLElement($raw);

  $feeds['title'] = (string)current($xml->xpath('//head/title'));

  // @todo: Make xpath case insensitive.
  $outlines = $xml->xpath('//outline[@xmlUrl]');
  foreach ($outlines as $outline) {
    $title = $url = NULL;
    foreach ($outline->attributes() as $k => $v) {
      if (strtolower($k) == 'title') {
        $title = (string) $v;
      }
      else if (strtolower($k) == 'xmlurl') {
        $url = (string) $v;
      }
      if ($title && $url) {
        $items[] = array(
          'title' => $title,
          'url' => $url,
        );
        break;
      }
    }
  }
  $feeds['items'] = $items;
  return $feeds;
}