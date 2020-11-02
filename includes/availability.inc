<?php

function rbge_cal_availability($year, $month, $day, $day_or_week = 'week'){
    
    // if there is no date passed use today
    if(!$year || !$month || !$day){
      $start_date = new DateTime();        
    }else{
      $start_date = DateTime::createFromFormat("Y-m-d", "$year-$month-$day");
    }

    // by default we set the end date to a weeks time
    if(!$day_or_week || $day_or_week == 'week'){
        $start_date->setTime(0,0,0); // start at beginning of first day
        $end_date = clone $start_date; // finish at another day
        $end_date->add(new DateInterval('P7D')); // which is seven days later
        $end_date->sub(new DateInterval('PT1S')); // knock a second off or we get all day events on the next day
  
        // if we move from daylight saving (fall back) then knock an hour off
        if($start_date->format('I') == 1 && $end_date->format('I') == 0){
            $end_date->sub(new DateInterval('PT1H')); 
        }
        
        // if we move to daylight saving (spring forward) add an hour on
        if($start_date->format('I') == 0 && $end_date->format('I') == 1){
            $end_date->add(new DateInterval('PT1H')); 
        }
        
        $end_date_displayed = clone $end_date; // but display a different end date
        $end_date_displayed->sub(new DateInterval('P1D')); // that is only 6 days later.
    }else{        
        $start_date->setTime(6,0,0); // start at six am
        $end_date = clone $start_date; // finish at another date on the same day
        $end_date->setTime(23,0,0); // finish at 11 pm
    }

    // set the title
    $title = 'Availability: ' .  $start_date->format('l jS F Y');
    if(isset($end_date_displayed)) $title .= ' to ' .  $end_date_displayed->format('l jS F Y');
    drupal_set_title( $title );
    
    // get the tree of events for the time period
    $period = array();
    $period['start'] = $start_date->format(DATE_ATOM);
    $period['start_timestamp'] = $start_date->getTimestamp();
    $period['end'] = $end_date->format(DATE_ATOM);
    $period['end_timestamp'] = $end_date->getTimestamp();
    $period['duration'] =  $period['end_timestamp']  - $period['start_timestamp'];
    
    module_load_include('inc', 'rbge_cal', 'includes/venue_event_tree');
    $venue_tree = get_venue_event_tree($period);
    
    $roots = $venue_tree[-1];
    unset($venue_tree[-1]);
        
    $out = '<table class="rbge-cal-availability">';
    
    $out .= '<tr>';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td class="rbge-cal-jump-to"> Jump to week starting: ';
    render_jump_to_week($out, $period);

    $week_link = '/rbge/availability';
    $today_link = '/rbge/availability/' . (new DateTime())->format('Y/m/d') . '/day';
    $out .= "&nbsp;|&nbsp;<button onclick=\"window.location = '$week_link'\">This Week</button>&nbsp;|&nbsp;<button onclick=\"window.location = '$today_link'\">Today</button>";
    $out .= '</td>';
    $out .= '</tr>';
    
    // we add the period heading to every row but render it differently
    $period_heading = get_period_heading($out, $period);
   
    $out .= '<tr class="rbge-cal-heading-row">';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td class="time-scales">';
    $out .= $period_heading;
    $out .= '</td>';
    $out .= '</tr>';
    
    foreach($roots as $root){
        render_venue_row($out, $root, $period, $period_heading, array());
    }
    $out .= '</table>';
    
    // add a span to use for the cursor time when it hovers 
    $out .= '<span id="cursor-time">Cursor Time</span>';
    
  //  dpm($venue_tree);
    
    return $out;
    
}

function render_jump_to_week(&$out, $period){
    
    $start = new DateTime($period['start']);
    $start_stamp = $start->format('W_Y');
    $start->sub(new DateInterval('P1Y'));
    $firstDayOfWeek = 1; // Monday
    $difference = ($firstDayOfWeek - $start->format('N'));
    if ($difference > 0) { $difference -= 7; }
    $start->modify("$difference days");
    
    
    $week = new DateInterval('P1W');
    
    $out .= "<select onchange=\"window.location=this.options[this.selectedIndex].value\" >";
    
    $year = false;
    for($i = 0; $i < 208; $i++){
        
        // advance it a week
        $start->add($week);
        
        $next_year = $start->format('Y'); 
        if($year != $next_year){
            // close the last one
            if($year){
              $out .= "</optgroup>";  
            }
            $year = $next_year;
            $out .= "<optgroup label=\"$year\" >";
        }
        
        // is it the current week
        if($start_stamp == $start->format('W_Y')) $selected = 'selected="true"';
        else $selected = '';
        
        // what is the value
        $value = 'value="/rbge/availability/'. $start->format('Y/m/d')  . '/week"';
        
        $out .= "<option $selected $value >";
        $out .= $start->format('jS F Y');
        $out .= '</option>';
    }
    
    $out .= '</optgroup>';
    $out .= '</select>';
    
}

/*
    This returns the html for the period heading and sections
    (because it will later be included many times)
    but writes the heading_heading to the out string
*/
function get_period_heading(&$out, $period){
    
    // we are in the hours range

    $heading = "";
    
    // put in the weeks header in no matter what
    render_weeks_heading($out, new DateTime($period['start']));

    // are we doing a day or multiple days?
    if($period['duration'] <= 86400){
        
        $div_duration = 60*60; // one hour in seconds
        $hours = $period['duration'] / $div_duration;
        $width = 100/$hours;
        
        $start = new DateTime($period['start']);
        
        // round it to the nearest hour
        $start->setTime($start->format('h'), 0, 0);
        
        // we are in the day range - so we need hours
        
        for($i = 0; $i < $hours; $i++){
            
            // calculate the offset
            $offset = $width * $i;
            
            $div_start = $start->format(DATE_ATOM);
            
            $heading .= '<div ';
            $heading .= 'data-start="'. $div_start .'" ';
            $heading .= 'data-duration="'. $div_duration .'" ';
            $heading .= 'class="period-heading" ';
            $heading .= 'style="width: '. $width  .'%; left: '. $offset .'%;">';
            $heading .=  $start->format('H \h\r\s');
            $heading .= "</div>";
            
            // move on an hour
            $start->add(new DateInterval('PT1H'));
            
        }
        
        // add in the heading_days with a week in it
        $out .= '<tr class="rbge-cal-heading-days-row">';
        $out .= '<td>&nbsp;</td>';
        $out .= '<td>&nbsp;</td>';
        $out .= '<td class="time-scales">';
        
        // from the beginning again
        $start = new DateTime($period['start']);
        $start->sub(new DateInterval('P3D'));
        $width = (1/7)*100;
        for($i = 0; $i < 7; $i++){
        
            // calculate the offset
            $offset = $width * $i;
            $out .= '<div class="period-heading' . ($i == 3 ? ' period-selected' : '') . '" style="width: '. $width  .'%; left: '. $offset .'%;">';
            $out .= '<a href="/rbge/availability/';
            $out .= $start->format('Y/m/d');
            $out .= '/day" >';
            $out .= $start->format('l jS');
            $out .= '</a>';
            $out .= "</div>";
    
            // move on a day
            $start->add(new DateInterval('P1D'));
        
        }
        
        $out .= '</td>';
        
    }else{

        // we are in the dayS range - so we need on day per
        $first_day = new DateTime($period['start']);
        $div_duration = 86400;
        $days = $period['duration'] / $div_duration;
        $width = ($div_duration / $period['duration']) * 100;
        
        for($i = 0; $i < $days; $i++){
            
            // calculate the offset
            $offset = $width * $i;
            $div_start = $first_day->format(DATE_ATOM);
            
            $heading .= '<div ';
            $heading .= 'data-start="'. $div_start .'" ';
            $heading .= 'data-duration="'. $div_duration .'" ';
            $heading .= 'class="period-heading" ';
            $heading .= 'style="width: '. $width  .'%; left: '. $offset .'%;">';
            $heading .= '<a href="/rbge/availability/';
            $heading .= $first_day->format('Y/m/d');
            $heading .= '/day" >';
            $heading .= $first_day->format('l jS');
            $heading .= '</a>';
            $heading .= "</div>";
            
            // move on a day
            $first_day->add(new DateInterval('P1D'));
            
        }
    }
    
    return $heading;

}

function render_weeks_heading(&$out, $a_date){
    
    // add a heading_weeks
    $out .= '<tr class="rbge-cal-heading-weeks-row">';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td>&nbsp;</td>';
    $out .= '<td class="time-scales">';
    
    $firstDayOfWeek = 1; // Monday
    $difference = ($firstDayOfWeek - $a_date->format('N'));
    if ($difference > 0) { $difference -= 7; }
    
    $width = 33.333333;

    $week_before = clone $a_date;
    $week_before->sub(new DateInterval('P7D'));
    $week_before->modify("$difference days");
    
    $out .= '<div class="period-heading" style="width: '. $width  .'%; left: 0%;">';
    $out .= '<a href="/rbge/availability/';
    $out .= $week_before->format('Y/m/d');
    $out .= '/week" >';
    $out .= 'Week: ' . $week_before->format('l jS F Y');
    $out .= '</a>';
    $out .= "</div>";

    $week_now = clone $a_date;
    $week_now->modify("$difference days");
    
    $out .= '<div class="period-heading period-selected" style="width: '. $width  .'%; left: '. $width .'%;">';
    $out .= '<a href="/rbge/availability/';
    $out .= $week_now->format('Y/m/d');
    $out .= '/week" >';
    $out .= 'Week: ' . $week_now->format('l jS F Y');
    $out .= '</a>';
    $out .= "</div>";
    
    $week_after = clone $a_date;
    $week_after->add(new DateInterval('P7D'));
    $week_after->modify("$difference days");
    
    $out .= '<div class="period-heading" style="width: '. $width  .'%; left: '. ($width * 2) .'%;">';
    $out .= '<a href="/rbge/availability/';
    $out .= $week_after->format('Y/m/d');
    $out .= '/week" >';
    $out .= 'Week: ' . $week_after->format('l jS F Y');
    $out .= '</a>';
    $out .= "</div>";
    $out .= '</td>';
    $out .= '</tr>';
    
}

function render_venue_row(&$out, $venue, $period, $period_heading, $classes){

    $venue_js_id = 'rbge-cal-venue-'. $venue->tid;
    $venue_depth_class = 'rbge-cal-venue-depth-' . $venue->depth;
    
    // if this is a top level venue then it is a garden and we add a special class to trickle down
    if($venue->depth == 0){
        switch ($venue->tid) {
            case 1:
                $classes[] = "rbge-cal-garden-edinburgh";
                break;
            case 2:
                $classes[] = "rbge-cal-garden-benmore";
                break;
            case 3:
                $classes[] = "rbge-cal-garden-logan";
                break;
            case 4:
                $classes[] = "rbge-cal-garden-dawyck";
                break;
        }
    }
    
    // start the row output
    $out .= '<tr data-venue-tid="' . $venue->tid . '" id="'. $venue_js_id . '" class="rbge-cal-venue-row ' . $venue_depth_class . ' ' . implode(' ', $classes) . '">';
    
    // venue switching on / off children
    $out .= ' <td class="rbge-cal-venue-switch">';
    if(isset($venue->children)){
        $out .= '<span class="rbge-cal-venue-switch-on">+</span>';
        $out .= '<span class="rbge-cal-venue-switch-off">&ndash;</span>';
    }
    $out .= '</td>';
    
    // venue name as a link
    $out .= '<td class="venue-names">';
    for($i = 0; $i < $venue->depth; $i++) $out .= '&nbsp;';
    if($venue->depth) $out .= ' ';
    $out .= '<a href="/taxonomy/term/';
    $out .= $venue->tid;
    $out .= '" >';
    $out .= $venue->name; // . " - " . $venue->tid;
    $out .= '</a>';
    
    
    // we include a link to print the weekly schedule out
    // but only on the week view
    if($period['duration'] ==  604799){
        $out .= '&nbsp;<a class="rbge-cal-schedule-link" href="/rbge/venue/schedule/';
        $out .= str_replace('-', '/', substr($period['start'], 0, 10));
        $out .= '/' . $venue->tid;
        $out .= '">';
        $out .= '[schedule]';
        $out .= '</a>';        
    }
    
    
    // close the venue name cell off
    $out .= '</td>';
    
    // the time scales - a single td filled with divs
    $out .= '<td class="time-scales">';
    
    // add background coloring as to whether the venue is closed or not
    $out .= render_closing_times($out, $classes, $period);
    
    // add the heading in first - css will place it behind the events as well
    $out .= $period_heading;
    
    if(isset($venue->events)){
        foreach($venue->events as $event_repeat){
            $event = $event_repeat[0];
            $repeat = $event_repeat[1];
            $proportions = get_event_repeat_proportions($repeat, $period);
            $onclick = "window.location = '/node/$event->nid'";
            $event_classes = "venue_repeats rbge-cal-event-status-" . $event->field_event_status['und'][0]['tid'] . " rbge-cal-category-" . $event->field_event_category['und'][0]['tid'];
            $out .= '<div onclick="'.$onclick.'" class="'. $event_classes  .'" style="width: '. $proportions['width']  .'%; left: '. $proportions['offset'] .'%;">';
            $out .= '<div class="venue_repeat_dialogue">';
            $out .= '<strong>';
            $out .= strlen($event->title) > 30 ?  substr($event->title, 0, 27) . ' ...' : $event->title;
            $out .= '</strong><br/>';
            $out .= (new DateTime($repeat->start))->format('H:i');    
            $out .= ' to ';
            $out .= (new DateTime($repeat->end))->format('H:i');
            $out .= '<br/>';
            $tpath = array_reverse(taxonomy_get_parents_all($event->field_event_category['und'][0]['tid']));
            $out .= $tpath[0]->name;
            if(isset($tpath[1])) $out .= ' > '. $tpath[1]->name;
            if(isset($tpath[2])) $out .= ' > ...';
            $out .= '</div>';
            $out .= '</div>';
        }
    }
    $out .= '</td>';
    
    $out .= '</tr>';
    
    
    // add rows for the children of this venue    
    if(isset($venue->children)){
        $classes[] = $venue_js_id . '-child'; // tag children as my children
        foreach($venue->children as $kid){
            render_venue_row($out, $kid, $period, $period_heading, $classes);
        }
    }
    
}

function render_closing_times(&$out, $classes, $period){
    
    $start_dt = new DateTime($period['start']);
    $end_dt = new DateTime($period['end']);
    
    // work out the garden tid (root parent of the tree)
    if( in_array('rbge-cal-garden-edinburgh', $classes) ) $garden = 'EDINBURGH';
    if( in_array('rbge-cal-garden-dawyck', $classes) ) $garden = 'DAWYCK';
    if( in_array('rbge-cal-garden-logan', $classes) ) $garden = 'LOGAN';
    if( in_array('rbge-cal-garden-benmore', $classes) ) $garden = 'BENMORE';

    
//    dpm( $venue,  );
    
    // if we are displaying a day or longer period
    if($period['duration'] <= 86400){
        
        // we are doing a day or less.

        $closing_times = garden_seasonal_closing($start_dt, $garden);
        
        $display_start_hr = $start_dt->format('G');
        $display_end_hr = $end_dt->format('G');
        $display_duration_hrs = $period['duration']/(60*60);

        
        // work through the closing times and render a div for each one
        foreach($closing_times as $ct){
            
            $start_hr = $ct[0];
            $end_hr = $ct[1];
            $duration_hrs = $end_hr - $start_hr +1;
            
            if($start_hr <= $display_start_hr){
                $left = 0;
            }else{
                $left = ( ($start_hr - $display_start_hr) /  $display_duration_hrs ) * 100;
            }
            
            if($start_hr < $display_start_hr){
                $duration_hrs = $duration_hrs - ($display_start_hr - $start_hr);
            }
            
            // the width is proportional to the hours scale but may exceed 100%
            $width = ($duration_hrs/$display_duration_hrs) * 100;
            if($left + $width > 100) $width = 100 - $left;
            
            render_closing_times_div($out, $left, $width);
        }
        
            
    }else{
        
        // we are rendering days so create a div for 
        // each day
        $first_day = clone $start_dt;
        $days = $period['duration'] / 86400;
        $width = (86400 / $period['duration']) * 100;
        for($i = 0; $i < $days; $i++){

            // calculate the offset
            $offset = $width * $i;
            
            $closing_times = garden_seasonal_closing($first_day, $garden);
            
            //if there is only one closing time it lasts all day!
            // otherwise we consider the garden open.
            if(count($closing_times) == 1){
                 render_closing_times_div($out, $offset, $width);
            }
            
            // move on a day
            $first_day->add(new DateInterval('P1D'));
        
        }
    }
 
    
}

function render_closing_times_div(&$out, $left, $width){
    $out .= '<div class="rbge-cal-garden-closed" style="left: '.$left.'%; width: '.$width.'%">garden closed</div>';
}

function garden_seasonal_closing($date, $garden){
    
    $day = $date->format('j');
    $month = $date->format('n');
    $dow = $date->format('w'); // 0 = sunday, 1 = monday etc
    
    //dpm($garden);
        
    switch ($garden) {

        case 'EDINBURGH':
            
            // two days a year closed completely
            if($day == 25 && $month == 12) return array(array(0, 23)); // Christmas day
            if($day == 1 && $month == 1) return array(array(0, 23)); // New Year's day
            
            if($month == 11 || $month == 12 || $month == 1){
                // winter opening times
                return array(array(0,9), array(16,23));
            }else{
                // summer opening
                return array(array(0,9), array(18,23));
            }
            break;

        case 'BENMORE':
        
            if($month < 4 || $month > 10){
                //  winter months closed
                return array(array(0, 23));
            }else{
                // summer opening
                // website says Open daily 1 March to 31 October from 10am to 5pm/6pm
                return array(array(0,9), array(18,23)); 
            }
            
            break;

        case 'DAWYCK':
        
            if($month < 2 || $month > 11){
                //  winter months closed
                return array(array(0, 23));
                
            }else{
                // summer opening
                // website says 1 February to 30 November from 10am to 4pm/5pm/6pm !!!
                return array(array(0,9), array(18,23)); 
            }
            
            break;
        
        case 'LOGAN':
        
            if(($month < 3 || $month > 10) || ($month == 3 && $day < 16)){
                
                if($month == 2 && $dow == 0){
                    // open Sundays in February
                    return array(array(0,9), array(17,23)); 
                }else{
                    //  winter months closed
                    return array(array(0, 23));
                }
                
            }else{
                // summer opening
                return array(array(0,9), array(17,23)); 
            }
            
            break;

    }
    
    // fall through to return nothing
    return array();
    
}

function get_event_repeat_proportions($event_period, $period){
    
    $event_start_dt = new DateTime($event_period->start); 
    $event_start_sec = $event_start_dt->getTimestamp();
    
    $event_end_dt = new DateTime($event_period->end);
    $event_end_sec = $event_end_dt->getTimestamp();
    
    // width is the proportion of the total period
    $diff = $event_end_sec  - $event_start_sec;
    $result['width'] = $diff/$period['duration'] * 100;
    
    // fudge for zero width - we must see the thing
    if($result['width'] < 1) $result['width'] = 0.5;
    
    // offest is the proportion the start is from the start of the period
    $offset_secs = $event_start_sec - $period['start_timestamp'];
    $result['offset'] =  $offset_secs/$period['duration'] * 100;
    
    // fudge for things that go beyond the current period
    if($result['width'] > 100){
        $result['width'] = 100;
        $result['offset'] = 0;
    } 
    if($result['offset'] < 0) $result['offset'] = 0;
    
    return $result;
    
}

