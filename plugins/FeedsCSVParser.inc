<?php
// $Id: FeedsCSVParser.inc,v 1.1 2009/09/16 22:08:48 alexb Exp $

/**
 * Parses a given file as a CSV file.
 */
class FeedsCSVParser extends FeedsParser {

  /**
   * Parses a raw string and returns a Feed object from it.
   */
  public function parse(FeedsFetcherResult $fetcherResult, $nid = NULL) {
    feeds_include('ParserCSV');

    if ($fetcherResult->type == 'text/filepath') {
      $iterator = new ParserCSVIterator(realpath($fetcherResult->value));
    }
    // @todo: write string buffer iterator.
    else {
      throw new Exception('You must use CSV Parser with File Fetcher.');
    }

    // Parse.
    $parser = new ParserCSV();
    $parser->setDelimiter($this->config['delimiter']);
    $parser->setSkipFirstLine(FALSE);
    $result = $parser->parse($iterator);
    return new FeedsParserResult($result);
  }

  /**
   * Return mapping sources.
   */
  public function getMappingSources() {
    return array();
  }

  /**
   * Define default configuration.
   */
  public function getDefaultConfig() {
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