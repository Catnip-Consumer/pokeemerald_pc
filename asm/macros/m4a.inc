	.macro song label:req, music_player:req, unknown:req
	.quad \label
	.short \music_player
	.short \unknown
    .space 4
	.endm

	.macro music_player info_struct:req, track_struct:req, num_tracks:req, unknown:req
	.quad \info_struct
	.quad \track_struct
	.byte \num_tracks
	.space 1  /* Padding*/
	.short \unknown
    .space 4  /* Padding*/
	.endm
