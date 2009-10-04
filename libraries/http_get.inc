<?php
// $Id$
/**
 * @file
 * Download via HTTP. 
 * 
 * Support caching, HTTP Basic Authentication, detection of RSS/Atom feeds, redirects.
 */

/**
 * Download RSS or Atom feeds from a given URL. If document in given URL is an HTML
 * document, function attempts to discover RSS or Atom feeds and downloads them.
 * 
 * @todo: use exceptions, not string or false return values.
 *
 * @return
 *  string - the downloaded data, FALSE - if the URL is not reachable
 */
function http_get_common_syndication($url, $settings = NULL) {
  if (valid_url($url, TRUE)) {
    // Handle password protected feeds.
    $url_parts = parse_url($url);
    $password = $username = NULL;
    if (!empty($url_parts['user'])) {
      $password = $url_parts['pass'];
      $username = $url_parts['user'];
    }
  }

  $accept_invalid_cert = isset($settings['accept_invalid_cert']) ? $settings['accept_invalid_cert'] : FALSE;
  $downloaded_string = http_get($url, $username, $password, $accept_invalid_cert);

  // Cannot get the feed, pass the problem to one level up.
  if ($downloaded_string == FALSE) {
    return FALSE;
  }
  // The data comes from cache, just pass one level up.
  else if (is_object($downloaded_string)) {
    return $downloaded_string;
  }

  // Do the autodiscovery at this level, pass back the real data.
  // Maybe it's HTML. If it's not HTML, not worth to take a look into the downloaded string.
  if (strpos(strtolower($downloaded_string), "<html") !== FALSE) {
    $allowed_mime = array("text/xml", "application/rss+xml", "application/atom+xml", "application/rdf+xml", "application/xml");
    $matches = array();
    // Get all the links tag
    preg_match_all('/<link\s+(.*?)\s*\/?>/si', $downloaded_string, $matches);
    $links = $matches[1];
    $rss_link = FALSE;
    foreach ($links as $link) {
      $mime = array();
      // Get the type attribute and check if the mime type is allowed.
      preg_match_all('/type\s*=\s*("|\')([A-Za-z\/+]*)("|\')/si', $link, $mime);
      if (in_array(array_pop($mime[2]), $allowed_mime)) {
        $href = array();
        // Get the href attribute.
        preg_match_all('/href\s*=\s*("|\')([=#\?_:.0-9A-Za-z\/+]*)("|\')/si', $link, $href);
        $rss_link = array_pop($href[2]);
        if (is_string($rss_link) && strlen($rss_link) > 0 && $rss_link != $url) {
          // Handle base url related stuff.
          $parsed_url = parse_url($rss_link);
          if (!isset($parsed_url['host'])) {
            // It's relative so make it absolute.
            $base_tag = array();
            preg_match_all('/<base href\s*=\s*("|\')([_:.0-9A-Za-z\/+]*)("|\')/si', $link, $base_tag);
            $base_url = array_pop($base_tag[2]);
            if (is_string($base_url) && strlen($base_url) > 0) {
              // Get from the HTML base tag.
              $rss_link = $base_url . $rss_link;
            }
            else {
              // Guess from the original URL.
              $original_url = parse_url($url);
              $rss_link = $original_url['scheme'] .'://'. $original_url['host'] . (isset($original_url['port']) ? ':' : '') . $original_url['port'] . $parsed_url['path'] .'?'. $parsed_url['query']  .'#'. $parsed_url['fragment'];
            }
          }
          $downloaded_string = http_get_common_syndication($rss_link);
          break;
        }
      }
    }
  }
  // Ugly hack to be able to retrieve the xml:base property, no way to access xml:lang inside <feed>
  $downloaded_string = preg_replace('/xml:base *=/', 'base=', $downloaded_string);

  // Filter out strange tags. Without this, the text would contain strange stuff.
  $downloaded_string_filtered = preg_replace(array('@<script[^>]*?.*?</script>@si', '@<object[^>]*?.*?</object>@si', '@<embed[^>]*?.*?</embed>@si', '@<applet[^>]*?.*?</applet>@si', '@<noframes[^>]*?.*?</noframes>@si', '@<noscript[^>]*?.*?</noscript>@si', '@<noembed[^>]*?.*?</noembed>@si'), '', $downloaded_string);
  return empty($downloaded_string_filtered) ? $downloaded_string : $downloaded_string_filtered;
}

/**
 * Get the content from the given URL.
 *
 * @param $url
 *  A valid URL (not only web URLs).
 * @param $username
 *  If the URL use authentication, here you can supply the username for this.
 * @param $password
 *  If the URL use authentication, here you can supply the password for this.
 * @return
 *  The data pulled from the URL or FALSE if the feed does not need refresh.
 */
function http_get($url, $username = NULL, $password = NULL, $accept_invalid_cert = FALSE) {
  // Intra-pagedownload cache, avoid to download the same content twice within one page download (it's possible, compatible and parse calls).
  static $download_cache = array();
  if (isset($download_cache[$url])) {
    return $download_cache[$url];
  }
  $has_etag = FALSE;
  $curl = http_get_use_curl();

  // Only download and parse data if really needs refresh.
  // Based on "Last-Modified" and "If-Modified-Since".
  $headers = array();
  if ($cache = cache_get('feeds_http_download_'. md5($url))) {
    $data = $cache->data;
    $has_etag = TRUE;
    if (!empty($data->etag)) {
      if ($curl) {
        $headers[] = 'If-None-Match: '. $data->etag;
      }
      else {
        $headers['If-None-Match'] = $data->etag;
      }
    }
    if (!empty($data->last_modified)) {
      if ($curl) {
        $headers[] = 'If-Modified-Since: '. $data->last_modified;
      }
      else {
        $headers['If-Modified-Since'] = $data->last_modified;
      }
    }
    if (!empty($username) && !$curl) {
      $headers['Authorization'] = 'Basic '. base64_encode("$username:$password");
    }
  }

  if ($curl) {
    $headers[] = 'User-Agent: Drupal (+http://drupal.org/)';
    $result = new stdClass();
    $download = curl_init($url);
    curl_setopt($download, CURLOPT_FOLLOWLOCATION, TRUE);
    if (!empty($username)) {
      curl_setopt($download, CURLOPT_USERPWD, "{$username}:{$password}");
    }
    curl_setopt($download, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($download, CURLOPT_HEADER, TRUE);
    curl_setopt($download, CURLOPT_RETURNTRANSFER, TRUE);
    if ($accept_invalid_cert) {
      curl_setopt($download, CURLOPT_SSL_VERIFYPEER, 0);
    }
    $header = '';
    $data = curl_exec($download);
    $header_size = curl_getinfo($download, CURLINFO_HEADER_SIZE);
    $header = substr($data, 0, $header_size - 1);
    $result->data = substr($data, $header_size);
    $header_lines = preg_split("/\r\n|\n|\r/", $header);

    $result->headers = array();
    array_shift($header_lines); // skip HTTP response status
    while ($line = trim(array_shift($header_lines))) {
      list($header, $value) = explode(':', $line, 2);
      if (isset($result->headers[$header]) && $header == 'Set-Cookie') {
        // RFC 2109: the Set-Cookie response header comprises the token Set-
        // Cookie:, followed by a comma-separated list of one or more cookies.
        $result->headers[$header] .= ','. trim($value);
      }
      else {
        $result->headers[$header] = trim($value);
      }
    }
    $result->code = curl_getinfo($download, CURLINFO_HTTP_CODE);

    curl_close($download);
  }
  else {
    $result = drupal_http_request($url, $headers);
  }

  $result->code = isset($result->code) ? $result->code : 200;
  // In this case return the cached data.
  if ($result->code == 304) {
    $cached_data = http_get($url);
    if (is_object($cached_data)) {
      $cached_data->from_cache = TRUE;
      return $cached_data;
    }
    else {
      // It's a tragedy, this file must exist and contain good data.
      // In this case, clear cache and repeat.
      cache_clear_all('feeds_http_download_'. md5($url));
      return http_get($url, $username, $password);
    }
  }

  if (!isset($result->headers) || !isset($result->headers['ETag']) || !isset($result->headers['Last-Modified'])) {
    $result->headers = isset($result->headers) ? $result->headers : array();
    $result->headers['ETag'] = isset($result->headers['ETag']) ? $result->headers['ETag'] : '';
    $result->headers['Last-Modified'] = isset($result->headers['Last-Modified']) ? $result->headers['Last-Modified'] : '';
  }

  // Set caches.
  $data = new stdClass();
  $data->etag = $result->headers['ETag'];
  $data->last_modified = $result->headers['Last-Modified'];

  cache_set('feeds_http_download_'. md5($url), $data);
  $download_cache[$url] = $result->data;

  return empty($result->data) ? FALSE : $result->data;
}

/**
 * Decides if it's possible to use cURL or not.
 *
 * @return
 *   TRUE if curl is available, FALSE otherwise.
 */
function http_get_use_curl() {
  $basedir = ini_get("open_basedir");
  return function_exists('curl_init') && !ini_get('safe_mode') && empty($basedir);
}