// $Id$

Drupal.behaviors.feeds = function() {

  // Export form machine-readable JS
  $('.feed-name:not(.processed)').each(function() {
    $('.feed-name')
      .addClass('processed')
      .after(' <small class="feed-id-suffix">&nbsp;</small>');
    if ($('.feed-id').val() === $('.feed-name').val().toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/_+/g, '_') || $('.feed-id').val() === '') {
      $('.feed-id').parents('.form-item').hide();
      $('.feed-name').keyup(function() {
        var machine = $(this).val().toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/_+/g, '_');
        if (machine !== '_' && machine !== '') {
          $('.feed-id').val(machine);
          $('.feed-id-suffix').empty().append(' Machine name: ' + machine + ' [').append($('<a href="#">'+ Drupal.t('Edit') +'</a>').click(function() {
            $('.feed-id').parents('.form-item').show();
            $('.feed-id-suffix').hide();
            $('.feed-name').unbind('keyup');
            return false;
          })).append(']');
        }
        else {
          $('.feed-id').val(machine);
          $('.feed-id-suffix').text('');
        }
      });
      $('.feed-name').keyup();
    }
  });

  // Hide text in specific input fields.
  $('.hide-text-on-focus').focus(function() {
    $(this).val('');
  });
};