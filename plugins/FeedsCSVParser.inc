<?php
// $Id: FeedsCSVParser.inc,v 1.1 2009/09/16 22:08:48 alexb Exp $

/**
 * Parses a given file as a CSV file.
 */
class FeedsCSVParser extends FeedsParser {

  /**
   * Parses a raw string and returns a Feed object from it.
   */
  public function parse(FeedsFetcherResult $fetcherResult, FeedsSource $source) {
    feeds_include_library('ParserCSV.inc', 'ParserCSV');

    if ($fetcherResult->type == 'text/filepath') {
      $iterator = new ParserCSVIterator(realpath($fetcherResult->value));
    }
    // @todo: write string buffer iterator.
    else {
      throw new Exception(t('You must use CSV Parser with File Fetcher.'));
    }

    // Parse.
    $parser = new ParserCSV();
    $parser->setDelimiter($this->config['delimiter']);
    $parser->setSkipFirstLine(FALSE);
    $rows = $parser->parse($iterator);

    // Apply titles in lower case.
    // @todo: push this functionality into ParserCSV.
    $header = array_shift($rows);
    foreach ($header as $i => $title) {
      $header[$i] = strtolower($title);
    }
    $result_rows = array();
    foreach ($rows as $i => $row) {
      $result_row = array();
      foreach ($row as $j => $col) {
        $result_row[$header[$j]] = $col;
      }
      $result_rows[$i] = $result_row;
    }
    unset($rows);

    // Return result.
    return new FeedsParserResult(array('items' => $result_rows));
  }

  /**
   * Define default configuration.
   */
  public function configDefaults() {
    return array('delimiter' => ',');
  }

  /**
   * Build configuration form.
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['delimiter'] = array(
      '#type' => 'select',
      '#title' => t('Delimiter'),
      '#description' => t('Field delimiter.'),
      '#options' => drupal_map_assoc(array(',', ';')),
      '#default_value' => $this->config['delimiter'],
    );
    return $form;
  }
}