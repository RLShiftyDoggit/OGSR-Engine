function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("effects_flare", "effects_flare")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.srcalpha,blend.one)
			: aref 		(true,2)
			: sorting	(3, false)
    shader:sampler 	("s_base")		:texture        (t_base)		:clamp()
end